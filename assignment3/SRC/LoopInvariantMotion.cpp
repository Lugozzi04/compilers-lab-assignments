//=============================================================================
// FILE:
//    LoopInvariantMotion.cpp
//
// DESCRIZIONE:
//    Pass LLVM semplice per la Loop-Invariant Code Motion.
//
//    Il pass:
//    1. Trova le istruzioni invarianti rispetto al loop.
//    2. Verifica se possono essere spostate in sicurezza.
//    3. Le sposta nel preheader del loop.
//
// UTILIZZO:
//    opt -S -load-pass-plugin ../BUILD/libLoopInvariantMotion.so \
//      -p loop-invariant-motion input.ll -o output.ll
//=============================================================================

#include "llvm/IR/PassManager.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/Dominators.h"

#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/LoopIterator.h"
#include "llvm/Analysis/ValueTracking.h"

#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/SetVector.h"

using namespace llvm;

namespace {

struct LoopInvariantMotion : PassInfoMixin<LoopInvariantMotion> {

    // -------------------------------------------------------------------------
    // Restituisce true se l'istruzione può essere considerata come possibile candidata.
    //
    // Manteniamo il pass conservativo:
    // - nessun terminatore;
    // - nessun nodo PHI da hoistare direttamente;
    // - nessuna istruzione con effetti collaterali;
    // - nessuna istruzione che legge memoria.
    // -------------------------------------------------------------------------
    bool isCandidateInstruction(Instruction &I) {
        if (I.isTerminator())
            return false;

        if (isa<PHINode>(&I))
            return false;

        if (I.mayHaveSideEffects())
            return false;

        if (I.mayReadFromMemory())
            return false;

        if (I.getType()->isVoidTy())
            return false;

        return true;
    }

    // -------------------------------------------------------------------------
    // Restituisce true se un valore è invariante rispetto al loop L.
    //
    // In LLVM SSA questo è l'equivalente pratico delle reaching definitions:
    // un valore è considerato invariante se la sua definizione non cambia
    // mentre siamo nel loop, oppure se è prodotta da un'istruzione già
    // riconosciuta come invarianta.
    //
    // Un valore è considerato invariante se:
    // - è una costante;
    // - è un argomento di funzione;
    // - è un valore globale;
    // - è definito al di fuori del loop;
    // - è definito all'interno del loop da un'istruzione già marcata come invariante.
    // -------------------------------------------------------------------------
    bool isLoopInvariantValue(Value *V, Loop *L, const SmallSetVector<Instruction *, 16> &InvariantInsts) {
        if (!V)
            return false;

        if (isa<Constant>(V))
            return true;

        if (isa<Argument>(V))
            return true;

        if (isa<GlobalValue>(V))
            return true;

        Instruction *DefInst = dyn_cast<Instruction>(V);

        if (!DefInst)
            return false;

        // If the value is defined outside the loop, it cannot change inside it.
        if (!L->contains(DefInst->getParent()))
            return true;

        // If it is defined inside the loop, it is invariant only if the defining
        // instruction has already been marked as invariant.
        if (InvariantInsts.count(DefInst))
            return true;

        return false;
    }

    // -------------------------------------------------------------------------
    // Le PHI possono essere invarianti anche se non vogliamo hoistarle.
    //
    // Questo aiuta la parte di analisi senza alterare il CFG in modo non valido.
    // -------------------------------------------------------------------------
    bool isLoopInvariantPhi(PHINode &PN, Loop *L, const SmallSetVector<Instruction *, 16> &InvariantInsts) {
        if (PN.getNumIncomingValues() == 0)
            return false;

        Value *FirstValue = PN.getIncomingValue(0);

        for (unsigned i = 1; i < PN.getNumIncomingValues(); ++i) {
            if (PN.getIncomingValue(i) != FirstValue)
                return false;
        }

        return isLoopInvariantValue(FirstValue, L, InvariantInsts);
    }

    // -------------------------------------------------------------------------
    // Restituisce true se l'istruzione è loop-invariant rispetto al loop.
    // -------------------------------------------------------------------------
    bool isLoopInvariantInstruction(Instruction &I, Loop *L, const SmallSetVector<Instruction *, 16> &InvariantInsts) {
        if (I.isTerminator())
            return false;

        if (auto *PN = dyn_cast<PHINode>(&I))
            return isLoopInvariantPhi(*PN, L, InvariantInsts);

        if (!isCandidateInstruction(I))
            return false;

        for (Use &U : I.operands()) {
            Value *Op = U.get();

            if (!isLoopInvariantValue(Op, L, InvariantInsts))
                return false;
        }

        return true;
    }

    // -------------------------------------------------------------------------
    // Verifica se il blocco che contiene I domina tutti i blocchi di uscita del loop.
    //
    // Questa è la condizione conservativa classica per la code motion:
    // l'istruzione è garantita essere eseguita prima di ogni possibile uscita dal loop.
    // -------------------------------------------------------------------------
    bool dominatesAllLoopExits(Instruction &I, Loop *L, DominatorTree &DT) {
        BasicBlock *InstBB = I.getParent();

        SmallVector<BasicBlock *, 8> ExitBlocks;
        L->getExitBlocks(ExitBlocks);

        if (ExitBlocks.empty())
            return false;

        for (BasicBlock *ExitBB : ExitBlocks) {
            if (!DT.dominates(InstBB, ExitBB))
                return false;
        }

        return true;
    }

    // -------------------------------------------------------------------------
    // Verifica se un'istruzione invariante può essere spostata nel preheader.
    //
    // Permettiamo lo spostamento se:
    // - l'istruzione domina tutte le uscite del loop;
    // OPPURE
    // - LLVM indica che è sicuro eseguirla speculativamente.
    // -------------------------------------------------------------------------
    bool isSafeToMove(Instruction &I, Loop *L, DominatorTree &DT) {
        if (isa<PHINode>(&I))
            return false;

        if (dominatesAllLoopExits(I, L, DT))
            return true;

        if (isSafeToSpeculativelyExecute(&I))
            return true;

        return false;
    }

    // -------------------------------------------------------------------------
    // Raccoglie le istruzioni invarianti del loop usando un ordine RPO.
    //
    // L'ordine RPO è più vicino alla traccia del corso:
    // prima i blocchi dominatori, poi i blocchi dominati. In questo modo è
    // naturale scoprire prima le definizioni e poi i loro usi.
    // -------------------------------------------------------------------------
    void collectLoopInvariantInstructions(Loop *L, LoopInfo &LI, SmallSetVector<Instruction *, 16> &InvariantInsts) {
        LoopBlocksRPO RPO(L);
        RPO.perform(&LI);

        for (BasicBlock *BB : RPO) {
            // Quando si processa un loop esterno, ignoriamo i blocchi che
            // appartengono ai loop interni: vengono analizzati ricorsivamente
            // prima del loop corrente.
            if (LI.getLoopFor(BB) != L)
                continue;

            for (Instruction &I : *BB) {
                if (!isLoopInvariantInstruction(I, L, InvariantInsts))
                    continue;

                InvariantInsts.insert(&I);

                errs() << "  Loop-invariant found: ";
                I.print(errs());
                errs() << "\n";
            }
        }
    }

    // -------------------------------------------------------------------------
    // Sposta l'istruzione I prima del terminator del preheader.
    // -------------------------------------------------------------------------
    void moveToPreheader(Instruction &I, BasicBlock *Preheader) {
        I.moveBefore(Preheader->getTerminator());
    }

    // -------------------------------------------------------------------------
    // Prova a hoistare le istruzioni invarianti già identificate.
    // -------------------------------------------------------------------------
    bool hoistLoopInvariantInstructions(Loop *L, DominatorTree &DT, SmallSetVector<Instruction *, 16> &InvariantInsts) {
        BasicBlock *Preheader = L->getLoopPreheader();

        if (!Preheader) {
            errs() << "Loop without preheader, skipped.\n";
            return false;
        }

        bool Changed = false;

        for (Instruction *I : InvariantInsts) {
            // Le PHI possono essere invarianti ma non vanno hoistate fuori dal
            // loro blocco: il loro ruolo è solo quello di aiutare l'analisi.
            if (isa<PHINode>(I))
                continue;

            if (I->getParent() == Preheader)
                continue;

            if (!isSafeToMove(*I, L, DT)) {
                errs() << "  Not safe to move: ";
                I->print(errs());
                errs() << "\n";
                continue;
            }

            errs() << "  Moving to preheader: ";
            I->print(errs());
            errs() << "\n";

            moveToPreheader(*I, Preheader);
            Changed = true;
        }

        return Changed;
    }

    // -------------------------------------------------------------------------
    // Processa un singolo loop.
    // -------------------------------------------------------------------------
    bool processLoop(Loop *L, LoopInfo &LI, DominatorTree &DT) {
        errs() << "Processing loop with header: ";
        L->getHeader()->printAsOperand(errs(), false);
        errs() << "\n";

        SmallSetVector<Instruction *, 16> InvariantInsts;
        collectLoopInvariantInstructions(L, LI, InvariantInsts);

        return hoistLoopInvariantInstructions(L, DT, InvariantInsts);
    }

    // -------------------------------------------------------------------------
    // Processa ricorsivamente i loop annidati.
    //
    // I loop interni sono processati prima di quelli esterni.
    // -------------------------------------------------------------------------
    bool processLoopRecursive(Loop *L, LoopInfo &LI, DominatorTree &DT) {
        bool Changed = false;

        for (Loop *SubLoop : L->getSubLoops()) {
            if (processLoopRecursive(SubLoop, LI, DT))
                Changed = true;
        }

        if (processLoop(L, LI, DT))
            Changed = true;

        return Changed;
    }

    // -------------------------------------------------------------------------
    // Punto d'ingresso del pass per funzione.
    // -------------------------------------------------------------------------
    PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM) {
        bool Changed = false;

        LoopInfo &LI = AM.getResult<LoopAnalysis>(F);
        DominatorTree &DT = AM.getResult<DominatorTreeAnalysis>(F);

        errs() << "========================================\n";
        errs() << "Function: " << F.getName() << "\n";
        errs() << "========================================\n";

        if (LI.empty()) {
            errs() << "No loops found.\n\n";
            return PreservedAnalyses::all();
        }

        for (Loop *L : LI) {
            if (processLoopRecursive(L, LI, DT))
                Changed = true;
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
// New Pass Manager plugin registration.
//-----------------------------------------------------------------------------
llvm::PassPluginLibraryInfo getLoopInvariantMotionPluginInfo() {
    return {
        LLVM_PLUGIN_API_VERSION,
        "LoopInvariantMotion",
        LLVM_VERSION_STRING,
        [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                     [](StringRef Name, FunctionPassManager &FPM, ArrayRef<PassBuilder::PipelineElement>) {
                    if (Name == "loop-invariant-motion") {
                        FPM.addPass(LoopInvariantMotion());
                        return true;
                    }

                    return false;
                });
        }
    };
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
    return getLoopInvariantMotionPluginInfo();
}
