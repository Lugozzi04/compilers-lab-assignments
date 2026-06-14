//=============================================================================
// FILE:
//    LoopFusion.cpp
//
// DESCRIZIONE:
//    Pass LLVM semplice per il Loop Fusion.
//
//    Il pass prova a fondere due loop adiacenti quando:
//      1. sono adiacenti;
//      2. hanno lo stesso trip count;
//      3. sono equivalenti nel controllo del flusso;
//      4. non ci sono dipendenze a distanza negativa.
//
// UTILIZZO:
//    opt -S -load-pass-plugin ../BUILD/libLoopFusion.so \
//      -p a4-loop-fusion input.ll -o output.ll
//=============================================================================

#include "llvm/IR/PassManager.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Dominators.h"

#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/DependenceAnalysis.h"

#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include "llvm/Support/raw_ostream.h"

#include <vector>

using namespace llvm;

namespace {

struct LoopFusion : PassInfoMixin<LoopFusion> {

    // -------------------------------------------------------------------------
    // Raccoglie solo istruzioni load/store.
    //
    // Questa scelta è intenzionalmente semplice e vicina agli esempi dell'esercizio.
    // -------------------------------------------------------------------------
    void collectMemoryInstructions(Loop *L, std::vector<Instruction *> &MemInsts) {
        MemInsts.clear();

        for (BasicBlock *BB : L->blocks()) {
            for (Instruction &I : *BB) {
                if (isa<LoadInst>(&I) || isa<StoreInst>(&I))
                    MemInsts.push_back(&I);
            }
        }
    }

    // -------------------------------------------------------------------------
    // Raccoglie i loop top-level nella loro ordine naturale.
    //
    // La consegna lavora su coppie di loop consecutivi allo stesso livello.
    // -------------------------------------------------------------------------
    void collectTopLevelLoops(LoopInfo &LI, std::vector<Loop *> &Loops) {
        Loops.clear();

        for (Loop *L : LI.getLoopsInPreorder()) {
            if (!L->getParentLoop())
                Loops.push_back(L);
        }
    }

    // -------------------------------------------------------------------------
    // Costruisce le coppie di loop consecutivi da testare.
    // -------------------------------------------------------------------------
    void collectCandidatePairs(const std::vector<Loop *> &Loops,
                               std::vector<std::pair<Loop *, Loop *>> &CandidatePairs) {
        CandidatePairs.clear();

        for (unsigned I = 0; I + 1 < Loops.size(); ++I)
            CandidatePairs.emplace_back(Loops[I], Loops[I + 1]);
    }

    // -------------------------------------------------------------------------
    // Verifica che la funzione non abbia nodi PHI in Exit.
    //
    // Questo mantiene semplice la riscrittura del CFG ed evita di produrre input PHI non validi.
    // -------------------------------------------------------------------------
    bool exitBlockHasNoPHI(BasicBlock *Exit) {
        if (!Exit)
            return false;

        for (Instruction &I : *Exit) {
            if (isa<PHINode>(&I))
                return false;
        }

        return true;
    }

    // -------------------------------------------------------------------------
    // Trova una variabile di induzione.
    //
    // Prima prova la variabile di induzione canonica di LLVM. Se non è disponibile,
    // cerca un nodo PHI nell'header del loop con valori in ingresso dal preheader e dal latch.
    // -------------------------------------------------------------------------
    PHINode *findInductionVariable(Loop *L) {
        if (PHINode *CanonicalIV = L->getCanonicalInductionVariable())
            return CanonicalIV;

        BasicBlock *Header = L->getHeader();
        BasicBlock *Preheader = L->getLoopPreheader();
        BasicBlock *Latch = L->getLoopLatch();

        if (!Header || !Preheader || !Latch)
            return nullptr;

        for (PHINode &Phi : Header->phis()) {
            bool HasIncomingFromPreheader = false;
            bool HasIncomingFromLatch = false;

            for (unsigned i = 0; i < Phi.getNumIncomingValues(); ++i) {
                BasicBlock *IncomingBlock = Phi.getIncomingBlock(i);

                if (IncomingBlock == Preheader)
                    HasIncomingFromPreheader = true;

                if (IncomingBlock == Latch)
                    HasIncomingFromLatch = true;
            }

            if (HasIncomingFromPreheader && HasIncomingFromLatch)
                return &Phi;
        }

        return nullptr;
    }

    // -------------------------------------------------------------------------
    // Controlla l'adiacenza.
    //
    // Versione conforme al PDF:
    //   - se i loop sono guarded, il successore non-loop del guard di L0
    //     deve coincidere con l'entry block di L1;
    //   - altrimenti, l'exit block di L0 deve coincidere con il preheader di L1.
    // -------------------------------------------------------------------------
    BasicBlock *getLoopEntryBlock(Loop *L) {
        if (BranchInst *Guard = L->getLoopGuardBranch())
            return Guard->getParent();

        return L->getLoopPreheader();
    }

    BasicBlock *getNonLoopGuardSuccessor(Loop *L) {
        BranchInst *Guard = L->getLoopGuardBranch();

        if (!Guard)
            return nullptr;

        for (unsigned i = 0; i < Guard->getNumSuccessors(); ++i) {
            BasicBlock *Succ = Guard->getSuccessor(i);

            if (!L->contains(Succ))
                return Succ;
        }

        return nullptr;
    }

    bool areAdjacent(Loop *L0, Loop *L1) {
        BasicBlock *Entry1 = getLoopEntryBlock(L1);

        if (!Entry1)
            return false;

        if (L0->getLoopGuardBranch()) {
            BasicBlock *GuardSucc0 = getNonLoopGuardSuccessor(L0);
            return GuardSucc0 && GuardSucc0 == Entry1;
        }

        BasicBlock *Exit0 = L0->getExitBlock();
        return Exit0 && Exit0 == Entry1;
    }

    // -------------------------------------------------------------------------
    // Controlla se due loop hanno lo stesso trip count usando ScalarEvolution.
    //
    // All'inizio usavamo getSmallConstantTripCount(), ma quello limita troppo
    // la fusione ai soli loop con bound piccolo e costante. Qui confrontiamo
    // invece il backedge-taken count esatto di SCEV, così restiamo più vicini
    // alla consegna e copriamo anche casi simbolici che LLVM sa comunque
    // modellare in modo preciso.
    // -------------------------------------------------------------------------
    bool haveSameTripCount(Loop *L0, Loop *L1, ScalarEvolution &SE) {
        if (!SE.hasLoopInvariantBackedgeTakenCount(L0) ||
            !SE.hasLoopInvariantBackedgeTakenCount(L1))
            return false;

        const SCEV *TC0 = SE.getBackedgeTakenCount(L0); // il backedge count è il numero di volte in cui si percorre il backedge; il trip count è di solito uno in più
        const SCEV *TC1 = SE.getBackedgeTakenCount(L1);

        if (isa<SCEVCouldNotCompute>(TC0) || isa<SCEVCouldNotCompute>(TC1))
            return false;

        return TC0 == TC1;
    }

    // -------------------------------------------------------------------------
    // Controlla l'equivalenza nel controllo del flusso.
    //
    // Versione semplice come nelle slide:
    //   Header0 domina Header1
    //   Header1 post-domina Header0
    // -------------------------------------------------------------------------
    bool areControlFlowEquivalent(Loop *L0, Loop *L1, DominatorTree &DT, PostDominatorTree &PDT) {
        BasicBlock *Header0 = L0->getHeader();
        BasicBlock *Header1 = L1->getHeader();

        if (!Header0 || !Header1)
            return false;

        return DT.dominates(Header0, Header1) &&
               PDT.dominates(Header1, Header0);
    }

    // -------------------------------------------------------------------------
    // Prova a rilevare dipendenze negative tra gli accessi dei due loop.
    //
    // Seguiamo le slide: DI.depends(...), poi guardiamo la direction vector.
    // Se in qualunque livello compare GT, la fusione viene rifiutata.
    // -------------------------------------------------------------------------
    bool hasNegativeDistanceDependence(Loop *L0, Loop *L1, DependenceInfo &DI) {
        std::vector<Instruction *> Mem0;
        std::vector<Instruction *> Mem1;

        collectMemoryInstructions(L0, Mem0);
        collectMemoryInstructions(L1, Mem1);

        for (Instruction *I0 : Mem0) {
            for (Instruction *I1 : Mem1) {
                std::unique_ptr<Dependence> Dep = DI.depends(I0, I1, true);

                if (!Dep)
                    continue;

                if (Dep->isConfused()) {
                    errs() << "  Confused dependence found between:\n";
                    errs() << "    ";
                    I0->print(errs());
                    errs() << "\n    ";
                    I1->print(errs());
                    errs() << "\n";
                    return true;
                }

                unsigned Levels = Dep->getLevels();

                for (unsigned Level = 1; Level <= Levels; ++Level) {
                    unsigned Direction = Dep->getDirection(Level);

                    if (Direction & Dependence::DVEntry::GT) {
                        errs() << "  Negative distance dependence found between:\n";
                        errs() << "    ";
                        I0->print(errs());
                        errs() << "\n    ";
                        I1->print(errs());
                        errs() << "\n";
                        return true;
                    }
                }
            }
        }

        return false;
    }

    // -------------------------------------------------------------------------
    // Restituisce il blocco body semplice di un loop.
    //
    // Per la forma canonica usata nell'esercizio:
    //
    //   header -> body -> latch -> header/exit
    //
    // il body è di solito il predecessore unico del latch.
    // -------------------------------------------------------------------------
    BasicBlock *getSimpleLoopBody(Loop *L) {
        BasicBlock *Latch = L->getLoopLatch();

        if (!Latch)
            return nullptr;

        BasicBlock *Body = Latch->getSinglePredecessor();

        if (!Body)
            return nullptr;

        if (!L->contains(Body))
            return nullptr;

        if (Body == L->getHeader())
            return nullptr;

        return Body;
    }

    // -------------------------------------------------------------------------
    // Restituisce il punto di inserimento ideale per le istruzioni fuse.
    //
    // Preferiamo l'incremento della IV del primo loop; se non lo troviamo,
    // inseriamo prima del terminator del latch.
    // -------------------------------------------------------------------------
    Instruction *getFusionInsertPoint(Loop *L, PHINode *IV) {
        BasicBlock *Latch = L->getLoopLatch();

        if (!Latch)
            return nullptr;

        if (IV) {
            if (Value *NextValue = IV->getIncomingValueForBlock(Latch)) {
                if (Instruction *NextInst = dyn_cast<Instruction>(NextValue))
                    return NextInst;
            }
        }

        return Latch->getTerminator();
    }

    // -------------------------------------------------------------------------
    // Raccoglie le istruzioni dal body del secondo loop che devono essere spostate
    // nel body del primo loop.
    //
    // Non spostiamo terminatori e nodi PHI.
    // -------------------------------------------------------------------------
    void collectMovableBodyInstructions(BasicBlock *Body, std::vector<Instruction *> &Insts) {
        Insts.clear();

        for (Instruction &I : *Body) {
            if (I.isTerminator())
                continue;

            if (isa<PHINode>(&I))
                continue;

            Insts.push_back(&I);
        }
    }

    // -------------------------------------------------------------------------
    // Reindirizza l'uscita del primo loop all'uscita del secondo loop.
    //
    // Dopo aver spostato il body di L1 dentro L0, il secondo loop diventa irraggiungibile.
    // Il blocco da riscrivere non è il latch, ma il blocco che effettivamente esce da L0.
    // -------------------------------------------------------------------------
    bool redirectFirstLoopExit(Loop *L0, BasicBlock *NewExit) {
        if (!NewExit)
            return false;

        BasicBlock *Exiting0 = L0->getExitingBlock();
        BasicBlock *OldExit = L0->getExitBlock();

        if (!Exiting0 || !OldExit)
            return false;

        BranchInst *BI = dyn_cast<BranchInst>(Exiting0->getTerminator());

        if (!BI)
            return false;

        bool Redirected = false;

        for (unsigned i = 0; i < BI->getNumSuccessors(); ++i) {
            BasicBlock *Succ = BI->getSuccessor(i);

            if (Succ == OldExit) {
                BI->setSuccessor(i, NewExit);
                Redirected = true;
            }
        }

        return Redirected;
    }

    // -------------------------------------------------------------------------
    // Fonde concretamente L0 e L1.
    //
    // Trasformazione semplificata:
    //   1. Trova IV0 e IV1.
    //   2. Sposta le istruzioni del body di L1 nel body di L0.
    //   3. Sostituisci gli usi di IV1 con IV0 solo dentro le istruzioni spostate.
    //   4. Reindirizza l'uscita di L0 all'uscita di L1.
    // -------------------------------------------------------------------------
    bool fuseLoops(Loop *L0, Loop *L1) {
        PHINode *IV0 = findInductionVariable(L0);
        PHINode *IV1 = findInductionVariable(L1);

        if (!IV0 || !IV1) {
            errs() << "  Cannot find induction variables.\n";
            return false;
        }

        BasicBlock *Body0 = getSimpleLoopBody(L0);
        BasicBlock *Body1 = getSimpleLoopBody(L1);

        if (!Body0 || !Body1) {
            errs() << "  Unsupported loop shape.\n";
            return false;
        }

        BasicBlock *Exit1 = L1->getExitBlock();

        if (!Exit1) {
            errs() << "  Second loop has no unique exit block.\n";
            return false;
        }

        if (!exitBlockHasNoPHI(Exit1)) {
            errs() << "  Second loop exit contains PHI nodes. Skipped.\n";
            return false;
        }

        std::vector<Instruction *> ToMove;
        collectMovableBodyInstructions(Body1, ToMove);

        if (ToMove.empty()) {
            errs() << "  No instructions to move from second loop body.\n";
            return false;
        }

        Instruction *InsertPoint = getFusionInsertPoint(L0, IV0);

        if (!InsertPoint) {
            errs() << "  Could not find insertion point.\n";
            return false;
        }

        errs() << "  Fusing loops...\n";

        for (Instruction *I : ToMove) {
            I->replaceUsesOfWith(IV1, IV0);
            I->moveBefore(InsertPoint);
        }

        if (!redirectFirstLoopExit(L0, Exit1)) {
            errs() << "  Could not redirect first loop exit.\n";
            return false;
        }

        errs() << "  Fusion applied.\n";
        return true;
    }

    // -------------------------------------------------------------------------
    // Controlla tutte le condizioni di fusione e, se possibile, fonde.
    // -------------------------------------------------------------------------
    bool tryFuseLoops(Loop *L0, Loop *L1, DominatorTree &DT, PostDominatorTree &PDT, ScalarEvolution &SE, DependenceInfo &DI) {
        errs() << "Checking loop pair:\n";
        errs() << "  L0 header: ";
        L0->getHeader()->printAsOperand(errs(), false);
        errs() << "\n  L1 header: ";
        L1->getHeader()->printAsOperand(errs(), false);
        errs() << "\n";

        if (L0->getParentLoop() != L1->getParentLoop()) {
            errs() << "  Not sibling loops.\n";
            return false;
        }

        if (!L0->isLoopSimplifyForm() || !L1->isLoopSimplifyForm()) {
            errs() << "  Loops are not in simplify form.\n";
            return false;
        }

        if (!areAdjacent(L0, L1)) {
            errs() << "  Not adjacent.\n";
            return false;
        }

        if (!haveSameTripCount(L0, L1, SE)) {
            errs() << "  Different trip count.\n";
            return false;
        }

        if (!areControlFlowEquivalent(L0, L1, DT, PDT)) {
            errs() << "  Not control-flow equivalent.\n";
            return false;
        }

        if (hasNegativeDistanceDependence(L0, L1, DI)) {
            errs() << "  Negative distance dependence. Cannot fuse.\n";
            return false;
        }

        return fuseLoops(L0, L1);
    }

    // -------------------------------------------------------------------------
    // Punto di ingresso del pass per funzione.
    // -------------------------------------------------------------------------
    PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM) {
        bool Changed = false;

        errs() << "========================================\n";
        errs() << "Function: " << F.getName() << "\n";
        errs() << "========================================\n";

        while (true) {
            LoopInfo &LI = AM.getResult<LoopAnalysis>(F);
            DominatorTree &DT = AM.getResult<DominatorTreeAnalysis>(F);
            PostDominatorTree &PDT = AM.getResult<PostDominatorTreeAnalysis>(F);
            ScalarEvolution &SE = AM.getResult<ScalarEvolutionAnalysis>(F);
            DependenceInfo &DI = AM.getResult<DependenceAnalysis>(F);

            std::vector<Loop *> Loops;
            std::vector<std::pair<Loop *, Loop *>> CandidatePairs;

            collectTopLevelLoops(LI, Loops);

            if (Loops.size() < 2) {
                if (!Changed)
                    errs() << "Not enough loops for fusion.\n";
                break;
            }

            collectCandidatePairs(Loops, CandidatePairs);

            bool FusedThisRound = false;

            // Prova le coppie di loop nell'ordine del programma.
            for (auto [L0, L1] : CandidatePairs) {
                if (tryFuseLoops(L0, L1, DT, PDT, SE, DI)) {
                    Changed = true;
                    FusedThisRound = true;
                    break;
                }
            }

            if (!FusedThisRound)
                break;

            // La fusione cambia la IR: invalidiamo le analisi e ripartiamo con
            // dati freschi, così possiamo valutare altre coppie residue.
            AM.invalidate(F, PreservedAnalyses::none());
        }

        errs() << "\n";

        if (Changed)
            return PreservedAnalyses::none();

        return PreservedAnalyses::all();
    }

    static bool isRequired() {
        return true;
    }
};

} // namespace

//-----------------------------------------------------------------------------
// Registrazione del plugin per il New Pass Manager.
//-----------------------------------------------------------------------------
llvm::PassPluginLibraryInfo getLoopFusionPluginInfo() {
    return {
        LLVM_PLUGIN_API_VERSION,
        "LoopFusion",
        LLVM_VERSION_STRING,
        [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name,
                   FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                    if (Name == "a4-loop-fusion") {
                        FPM.addPass(LoopFusion());
                        return true;
                    }

                    return false;
                });
        }
    };
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
    return getLoopFusionPluginInfo();
}
