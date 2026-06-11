#include "llvm/IR/PassManager.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Constants.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace {

struct MultiInstructionOpt : PassInfoMixin<MultiInstructionOpt> {

    PreservedAnalyses run(Function &F, FunctionAnalysisManager &) {
        bool Changed = false;

        for (BasicBlock &B : F) {
            for (Instruction &I : B) {

                BinaryOperator *CurrOp = dyn_cast<BinaryOperator>(&I);
                if (!CurrOp)
                    continue;

                Value *CurrOp0 = CurrOp->getOperand(0);
                Value *CurrOp1 = CurrOp->getOperand(1);

                // =========================================================
                // CASO 1:
                //
                // a = x + k
                // c = a - k
                //
                // oppure:
                //
                // a = k + x
                // c = a - k
                //
                // Risultato:
                //
                // c = x
                // =========================================================
                if (CurrOp->getOpcode() == Instruction::Sub) {

                    ConstantInt *SubConst = dyn_cast<ConstantInt>(CurrOp1);
                    if (!SubConst)
                        continue;

                    BinaryOperator *PrevOp = dyn_cast<BinaryOperator>(CurrOp0);
                    if (!PrevOp)
                        continue;

                    if (PrevOp->getOpcode() != Instruction::Add)
                        continue;

                    Value *AddOp0 = PrevOp->getOperand(0);
                    Value *AddOp1 = PrevOp->getOperand(1);

                    // Pattern:
                    // a = x + k
                    // c = a - k
                    if (ConstantInt *AddConst = dyn_cast<ConstantInt>(AddOp1)) {
                        if (AddConst->getValue() == SubConst->getValue()) {
                            CurrOp->replaceAllUsesWith(AddOp0);
                            Changed = true;
                            continue;
                        }
                    }

                    // Pattern:
                    // a = k + x
                    // c = a - k
                    if (ConstantInt *AddConst = dyn_cast<ConstantInt>(AddOp0)) {
                        if (AddConst->getValue() == SubConst->getValue()) {
                            CurrOp->replaceAllUsesWith(AddOp1);
                            Changed = true;
                            continue;
                        }
                    }
                }

                // =========================================================
                // CASO 2:
                //
                // a = x - k
                // c = a + k
                //
                // Risultato:
                //
                // c = x
                // =========================================================
                if (CurrOp->getOpcode() == Instruction::Add) {

                    BinaryOperator *PrevOp = nullptr;
                    ConstantInt *AddConst = nullptr;

                    // Pattern:
                    // c = a + k
                    if ((PrevOp = dyn_cast<BinaryOperator>(CurrOp0))) {
                        AddConst = dyn_cast<ConstantInt>(CurrOp1);
                    }

                    // Pattern:
                    // c = k + a
                    if (!PrevOp) {
                        PrevOp = dyn_cast<BinaryOperator>(CurrOp1);
                        AddConst = dyn_cast<ConstantInt>(CurrOp0);
                    }

                    if (!PrevOp || !AddConst)
                        continue;

                    if (PrevOp->getOpcode() != Instruction::Sub)
                        continue;

                    Value *SubOp0 = PrevOp->getOperand(0);
                    Value *SubOp1 = PrevOp->getOperand(1);

                    ConstantInt *SubConst = dyn_cast<ConstantInt>(SubOp1);
                    if (!SubConst)
                        continue;

                    // Pattern:
                    // a = x - k
                    // c = a + k
                    if (SubConst->getValue() == AddConst->getValue()) {
                        CurrOp->replaceAllUsesWith(SubOp0);
                        Changed = true;
                        continue;
                    }
                }
            }
        }

        if (Changed)
            return PreservedAnalyses::none();

        return PreservedAnalyses::all();
    }

    static bool isRequired() { return true; }
};

} // namespace

llvm::PassPluginLibraryInfo getMultiInstructionOptPluginInfo() {
    return {
        LLVM_PLUGIN_API_VERSION,
        "MultiInstructionOpt",
        LLVM_VERSION_STRING,
        [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name,
                   FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                    if (Name == "multi-inst-opt") {
                        FPM.addPass(MultiInstructionOpt());
                        return true;
                    }
                    return false;
                });
        }
    };
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
    return getMultiInstructionOptPluginInfo();
}