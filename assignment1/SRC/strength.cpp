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

struct StrengthReduction : PassInfoMixin<StrengthReduction> {

    PreservedAnalyses run(Function &F, FunctionAnalysisManager &) {
        bool Changed = false;

        for (auto BBIter = F.begin(); BBIter != F.end(); ++BBIter) {
            BasicBlock &B = *BBIter;

            for (auto InstIter = B.begin(); InstIter != B.end(); ++InstIter) {
                Instruction &I = *InstIter;

                auto *BO = dyn_cast<BinaryOperator>(&I);
                if (!BO)
                    continue;

                Value *Op0 = BO->getOperand(0);
                Value *Op1 = BO->getOperand(1);

                // ---------------------------------------------------------
                // Caso 1:
                // x * 15  oppure  15 * x
                // diventa:
                // (x << 4) - x
                // ---------------------------------------------------------
                if (BO->getOpcode() == Instruction::Mul) {
                    Value *X = nullptr;

                    // Caso: 15 * x
                    if (auto *C = dyn_cast<ConstantInt>(Op0)) {
                        if (C->getSExtValue() == 15) {
                            X = Op1;
                        }
                    }

                    // Caso: x * 15
                    if (auto *C = dyn_cast<ConstantInt>(Op1)) {
                        if (C->getSExtValue() == 15) {
                            X = Op0;
                        }
                    }

                    if (X) {
                        Value *ShiftAmount = ConstantInt::get(X->getType(), 4);

                        // x << 4 = x * 16
                        Instruction *Shift = BinaryOperator::Create(Instruction::Shl, X, ShiftAmount, "mul15.shift");

                        Shift->insertBefore(BO);

                        // (x << 4) - x = 16x - x = 15x
                        Instruction *Sub = BinaryOperator::Create(Instruction::Sub, Shift, X, "mul15.sub");

                        Sub->insertBefore(BO);

                        BO->replaceAllUsesWith(Sub);

                        Changed = true;
                        continue;
                    }
                }

                // ---------------------------------------------------------
                // Caso 2:
                // unsigned x / 8
                // diventa:
                // x >> 3
                //
                // In LLVM: udiv x, 8  ->  lshr x, 3
                // ---------------------------------------------------------
                if (BO->getOpcode() == Instruction::UDiv) {
                    if (auto *C = dyn_cast<ConstantInt>(Op1)) {
                        if (C->getSExtValue() == 8) {
                            Value *ShiftAmount =
                                ConstantInt::get(Op0->getType(), 3);

                            // x / 8 = x >> 3 per unsigned

                            Instruction *Shift = BinaryOperator::Create(Instruction::LShr, Op0, ShiftAmount, "div8.shift");

                            Shift->insertBefore(BO);

                            BO->replaceAllUsesWith(Shift);

                            Changed = true;
                            continue;
                        }
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

llvm::PassPluginLibraryInfo getStrengthReductionPluginInfo() {
    return {
        LLVM_PLUGIN_API_VERSION,
        "StrengthReduction",
        LLVM_VERSION_STRING,
        [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name,
                   FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                    if (Name == "strength-reduction") {
                        FPM.addPass(StrengthReduction());
                        return true;
                    }
                    return false;
                });
        }
    };
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
    return getStrengthReductionPluginInfo();
}