; ModuleID = 'test.O0.ll'
source_filename = "test.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; Function Attrs: noinline nounwind uwtable
define dso_local i32 @test_algebraic(i32 noundef %0) #0 {
  %2 = add nsw i32 %0, 0
  %3 = add nsw i32 0, %2
  %4 = mul nsw i32 %3, 1
  %5 = mul nsw i32 1, %4
  ret i32 %5
}

; Function Attrs: noinline nounwind uwtable
define dso_local i32 @test_multi(i32 noundef %0) #0 {
  %2 = add nsw i32 %0, 1
  %3 = sub nsw i32 %2, 1
  ret i32 %3
}

; Function Attrs: noinline nounwind uwtable
define dso_local i32 @test_strength_mul_left(i32 noundef %0) #0 {
  %2 = mul nsw i32 %0, 15
  ret i32 %2
}

; Function Attrs: noinline nounwind uwtable
define dso_local i32 @test_strength_mul_right(i32 noundef %0) #0 {
  %2 = mul nsw i32 15, %0
  ret i32 %2
}

; Function Attrs: noinline nounwind uwtable
define dso_local i32 @test_strength_div8(i32 noundef %0) #0 {
  %2 = udiv i32 %0, 8
  ret i32 %2
}

; Function Attrs: noinline nounwind uwtable
define dso_local i32 @main() #0 {
  %1 = call i32 @test_algebraic(i32 noundef 10)
  %2 = call i32 @test_multi(i32 noundef 20)
  %3 = call i32 @test_strength_mul_left(i32 noundef 3)
  %4 = call i32 @test_strength_mul_right(i32 noundef 4)
  %5 = call i32 @test_strength_div8(i32 noundef 64)
  %6 = add nsw i32 %1, %2
  %7 = add nsw i32 %6, %3
  %8 = add nsw i32 %7, %4
  %9 = add i32 %8, %5
  ret i32 %9
}

attributes #0 = { noinline nounwind uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }

!llvm.module.flags = !{!0, !1, !2, !3, !4}
!llvm.ident = !{!5}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 8, !"PIC Level", i32 2}
!2 = !{i32 7, !"PIE Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 2}
!4 = !{i32 7, !"frame-pointer", i32 2}
!5 = !{!"clang version 19.1.7 (/home/runner/work/llvm-project/llvm-project/clang cd708029e0b2869e80abe31ddb175f7c35361f90)"}
