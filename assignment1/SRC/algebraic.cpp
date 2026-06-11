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

struct AlgebraicIdentity : PassInfoMixin<AlgebraicIdentity> {

    PreservedAnalyses run(Function &F, FunctionAnalysisManager &) {
        bool Changed = false;

        for (BasicBlock &B : F) {
            for (Instruction &I : B) {

                BinaryOperator *BinOp = dyn_cast<BinaryOperator>(&I);
                if (!BinOp)
                    continue;

                Value *Op0 = BinOp->getOperand(0);
                Value *Op1 = BinOp->getOperand(1);

                // ---------------------------------------------------------
                // CASO ADDIZIONE
                //
                // x + 0 -> x
                // 0 + x -> x
                // ---------------------------------------------------------
                if (BinOp->getOpcode() == Instruction::Add) {

                    // 0 + x -> x
                    if (ConstantInt *C = dyn_cast<ConstantInt>(Op0)) {
                        if (C->getValue().isZero()) {
                            BinOp->replaceAllUsesWith(Op1);
                            Changed = true;
                            continue;
                        }
                    }

                    // x + 0 -> x
                    if (ConstantInt *C = dyn_cast<ConstantInt>(Op1)) {
                        if (C->getValue().isZero()) {
                            BinOp->replaceAllUsesWith(Op0);
                            Changed = true;
                            continue;
                        }
                    }
                }

                // ---------------------------------------------------------
                // CASO SOTTRAZIONE
                //
                // x - 0 -> x
                //
                // NON ottimizziamo 0 - x, perché non è equivalente a x.
                // ---------------------------------------------------------
                if (BinOp->getOpcode() == Instruction::Sub) {

                    if (ConstantInt *C = dyn_cast<ConstantInt>(Op1)) {
                        if (C->getValue().isZero()) {
                            BinOp->replaceAllUsesWith(Op0);
                            Changed = true;
                            continue;
                        }
                    }
                }

                // ---------------------------------------------------------
                // CASO MOLTIPLICAZIONE
                //
                // x * 1 -> x
                // 1 * x -> x
                // ---------------------------------------------------------
                if (BinOp->getOpcode() == Instruction::Mul) {

                    // 1 * x -> x
                    if (ConstantInt *C = dyn_cast<ConstantInt>(Op0)) {
                        if (C->getValue().isOne()) {
                            BinOp->replaceAllUsesWith(Op1);
                            Changed = true;
                            continue;
                        }
                    }

                    // x * 1 -> x
                    if (ConstantInt *C = dyn_cast<ConstantInt>(Op1)) {
                        if (C->getValue().isOne()) {
                            BinOp->replaceAllUsesWith(Op0);
                            Changed = true;
                            continue;
                        }
                    }
                }

                // ---------------------------------------------------------
                // CASO DIVISIONE
                //
                // x / 1 -> x
                //
                // Valido sia per la divisione intera con segno che senza segno.
                //
                // NON ottimizziamo 0 / x -> 0, perché se x è 0 il
                // programma originale ha una divisione per zero.
                // ---------------------------------------------------------
                if (BinOp->getOpcode() == Instruction::SDiv ||
                    BinOp->getOpcode() == Instruction::UDiv) {

                    if (ConstantInt *C = dyn_cast<ConstantInt>(Op1)) {
                        if (C->getValue().isOne()) {
                            BinOp->replaceAllUsesWith(Op0);
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

llvm::PassPluginLibraryInfo getAlgebraicIdentityPluginInfo() {
    return {
        LLVM_PLUGIN_API_VERSION,
        "AlgebraicIdentity",
        LLVM_VERSION_STRING,
        [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name,
                   FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                    if (Name == "algebraic-identity") {
                        FPM.addPass(AlgebraicIdentity());
                        return true;
                    }
                    return false;
                });
        }
    };
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
    return getAlgebraicIdentityPluginInfo();
}