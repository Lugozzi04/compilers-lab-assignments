#!/bin/bash

set -e

export PATH="$HOME/tools/llvm-19.1.7/bin:$PATH"

echo "[1] Generating LLVM IR from test.c"
clang -O0 -Xclang -disable-O0-optnone -emit-llvm -S -c test.c -o test.O0.ll

echo "[2] Running mem2reg"
opt -passes=mem2reg test.O0.ll -S -o test.m2r.ll

echo "[3] Running custom optimization passes"
opt -S \
  -load-pass-plugin ../BUILD/libAlgebraicIdentity.so \
  -load-pass-plugin ../BUILD/libMultiInstructionOpt.so \
  -load-pass-plugin ../BUILD/libStrengthReduction.so \
  -p "algebraic-identity,multi-inst-opt,strength-reduction" \
  test.m2r.ll \
  -o test.optimized.ll

echo "[4] Running Dead Code Elimination"
opt -S \
  -passes=dce \
  test.optimized.ll \
  -o test.final.ll

echo "[5] Diff original m2r vs optimized before DCE"
diff -u test.m2r.ll test.optimized.ll || true

echo "[6] Diff optimized before DCE vs final after DCE"
diff -u test.optimized.ll test.final.ll || true

echo "Done."
echo "Intermediate output: test.optimized.ll"
echo "Final output:        test.final.ll"