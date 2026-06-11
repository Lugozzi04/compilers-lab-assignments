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
#include "llvm/Analysis/ScalarEvolutionExpressions.h"
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
    // Restituisce l'operando puntatore di un'istruzione load/store.
    // -------------------------------------------------------------------------
    Value *getPointerOperand(Instruction *I) {
        if (auto *LI = dyn_cast<LoadInst>(I))
            return LI->getPointerOperand();

        if (auto *SI = dyn_cast<StoreInst>(I))
            return SI->getPointerOperand();

        return nullptr;
    }

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
    // Restituisce il blocco di ingresso usato per i controlli di dominanza/post-dominanza.
    //
    // Se il loop è guarded, usa il blocco di guardia.
    // Altrimenti, usa il preheader del loop.
    // -------------------------------------------------------------------------
    BasicBlock *getLoopEntryForChecks(Loop *L) {
        if (BranchInst *Guard = L->getLoopGuardBranch())
            return Guard->getParent();

        return L->getLoopPreheader();
    }

    // -------------------------------------------------------------------------
    // Controlla l'adiacenza.
    //
    // Caso non guarded:
    //   exit block di L0 == preheader di L1
    //
    // Caso guarded:
    //   il successore fuori dal loop della catena guard/exit di L0 raggiunge l'ingresso di L1.
    //
    // Resta comunque un controllo semplificato, ma gestisce i casi più comuni
    // discussi nell'esercizio.
    // -------------------------------------------------------------------------
    bool areAdjacent(Loop *L0, Loop *L1) {
        BranchInst *Guard0 = L0->getLoopGuardBranch();
        BranchInst *Guard1 = L1->getLoopGuardBranch();

        bool IsGuarded0 = Guard0 != nullptr;
        bool IsGuarded1 = Guard1 != nullptr;

        if (IsGuarded0 != IsGuarded1)
            return false;

        BasicBlock *Entry1 = getLoopEntryForChecks(L1);

        if (!Entry1)
            return false;

        if (!IsGuarded0) {
            BasicBlock *Exit0 = L0->getExitBlock();
            BasicBlock *Preheader1 = L1->getLoopPreheader();

            return Exit0 && Preheader1 && Exit0 == Preheader1;
        }

        BasicBlock *GuardBlock0 = Guard0->getParent();

        for (unsigned i = 0; i < Guard0->getNumSuccessors(); ++i) {
            BasicBlock *Succ = Guard0->getSuccessor(i);

            if (!L0->contains(Succ) && Succ == Entry1)
                return true;
        }

        // Fallback: controlla il blocco di uscita unico.
        BasicBlock *Exit0 = L0->getExitBlock();
        return Exit0 && Exit0 == Entry1 && GuardBlock0;
    }

    // -------------------------------------------------------------------------
    // Controlla se due loop hanno lo stesso trip count usando ScalarEvolution.
    //
    // Usiamo getBackedgeTakenCount invece di getSmallConstantTripCount perché
    // funziona anche con trip count simbolici come n.
    // -------------------------------------------------------------------------
    bool haveSameTripCount(Loop *L0, Loop *L1, ScalarEvolution &SE) {
        const SCEV *TC0 = SE.getBackedgeTakenCount(L0);
        const SCEV *TC1 = SE.getBackedgeTakenCount(L1);

        if (isa<SCEVCouldNotCompute>(TC0) || isa<SCEVCouldNotCompute>(TC1))
            return false;

        return TC0 == TC1;
    }

    // -------------------------------------------------------------------------
    // Controlla l'equivalenza nel controllo del flusso.
    //
    // L0 e L1 sono equivalenti nel controllo del flusso se:
    //   L0 domina L1
    //   L1 post-domina L0
    // -------------------------------------------------------------------------
    bool areControlFlowEquivalent(Loop *L0, Loop *L1, DominatorTree &DT, PostDominatorTree &PDT) {
        BasicBlock *Entry0 = getLoopEntryForChecks(L0);
        BasicBlock *Entry1 = getLoopEntryForChecks(L1);

        if (!Entry0 || !Entry1)
            return false;

        return DT.dominates(Entry0, Entry1) &&
               PDT.dominates(Entry1, Entry0);
    }

    // -------------------------------------------------------------------------
    // Prova a rilevare una dipendenza a distanza negativa usando SCEV.
    //
    // Analizziamo solo accessi semplici ad array di tipo affine rappresentati come SCEVAddRecExpr.
    //
    // Esempio che dovrebbe impedire la fusione:
    //
    //   Loop 0: A[i]   = ...
    //   Loop 1: ... = A[i+3]
    //
    // Store - Load dà una distanza negativa, il che significa che il secondo loop usa
    // un valore prodotto da un'iterazione futura del primo loop.
    // -------------------------------------------------------------------------
    bool hasNegativeDistanceDependence(Loop *L0, Loop *L1, ScalarEvolution &SE, DependenceInfo &DI) {
        std::vector<Instruction *> Mem0;
        std::vector<Instruction *> Mem1;

        collectMemoryInstructions(L0, Mem0);
        collectMemoryInstructions(L1, Mem1);

        for (Instruction *I0 : Mem0) {
            for (Instruction *I1 : Mem1) {

                std::unique_ptr<Dependence> Dep = DI.depends(I0, I1, true);

                if (!Dep)
                    continue;

                Value *Ptr0 = getPointerOperand(I0);
                Value *Ptr1 = getPointerOperand(I1);

                if (!Ptr0 || !Ptr1)
                    return true;

                const SCEV *S0 = SE.getSCEVAtScope(Ptr0, L0);
                const SCEV *S1 = SE.getSCEVAtScope(Ptr1, L1);

                const SCEVAddRecExpr *AR0 = dyn_cast<SCEVAddRecExpr>(S0);
                const SCEVAddRecExpr *AR1 = dyn_cast<SCEVAddRecExpr>(S1);

                // Se c'è una dipendenza ma non riusciamo a ragionarci sopra, meglio essere conservativi.
                if (!AR0 || !AR1)
                    return true;

                const SCEV *Step0 = AR0->getStepRecurrence(SE);
                const SCEV *Step1 = AR1->getStepRecurrence(SE);

                if (Step0 != Step1)
                    return true;

                const SCEV *Start0 = AR0->getStart();
                const SCEV *Start1 = AR1->getStart();

                const SCEV *Distance = SE.getMinusSCEV(Start0, Start1);

                // Some pointer-shaped SCEVs do not support a meaningful signed range
                // query here. If we cannot reason about the distance safely, stay
                // conservative and reject the fusion instead of crashing.
                if (!Distance->getType()->isIntegerTy())
                    return true;

                if (SE.isKnownNegative(Distance)) {
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

        Instruction *InsertPoint = Body0->getTerminator();

        errs() << "  Fusing loops...\n";

        for (Instruction *I : ToMove) {
            I->replaceUsesOfWith(IV1, IV0);
            I->moveBefore(InsertPoint);
        }

        if (!redirectFirstLoopExit(L0, Exit1)) {
            errs() << "  Could not redirect first loop exit.\n";
            return false;
        }

        // Rimuove i blocchi del secondo loop che sono diventati irraggiungibili.
        Function *F = L0->getHeader()->getParent();
        if (F)
            EliminateUnreachableBlocks(*F, nullptr, true);

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

        if (hasNegativeDistanceDependence(L0, L1, SE, DI)) {
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

        LoopInfo &LI = AM.getResult<LoopAnalysis>(F);
        DominatorTree &DT = AM.getResult<DominatorTreeAnalysis>(F);
        PostDominatorTree &PDT = AM.getResult<PostDominatorTreeAnalysis>(F);
        ScalarEvolution &SE = AM.getResult<ScalarEvolutionAnalysis>(F);
        DependenceInfo &DI = AM.getResult<DependenceAnalysis>(F);

        errs() << "========================================\n";
        errs() << "Function: " << F.getName() << "\n";
        errs() << "========================================\n";

        std::vector<Loop *> Loops;

        for (Loop *L : LI.getLoopsInPreorder()) {
            Loops.push_back(L);
        }

        if (Loops.size() < 2) {
            errs() << "Not enough loops for fusion.\n\n";
            return PreservedAnalyses::all();
        }

        // Prova le coppie di loop nell'ordine del programma.
        for (unsigned i = 0; i + 1 < Loops.size(); ++i) {
            Loop *L0 = Loops[i];
            Loop *L1 = Loops[i + 1];

            if (tryFuseLoops(L0, L1, DT, PDT, SE, DI)) {
                Changed = true;
                break;
            }
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
