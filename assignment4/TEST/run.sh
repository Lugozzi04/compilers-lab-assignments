
clang -O0 -Xclang -disable-O0-optnone -emit-llvm -S -c test.c -o test.O0.ll
opt -passes=mem2reg test.O0.ll -S -o test.m2r.ll
opt -S \
  -load-pass-plugin ../BUILD/libLoopFusion.so \
  -p a4-loop-fusion \
  test.m2r.ll \
  -o test.optimized.ll
opt -S \
  -passes=dce \
  test.optimized.ll \
  -o test.final.ll