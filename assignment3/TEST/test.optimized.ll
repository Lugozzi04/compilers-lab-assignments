; ModuleID = 'test.m2r.ll'
source_filename = "test.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; Function Attrs: noinline nounwind uwtable
define dso_local void @test_basic(ptr noundef %0, i32 noundef %1, i32 noundef %2, i32 noundef %3) #0 {
  %5 = mul nsw i32 %2, %3
  %6 = add nsw i32 %5, 10
  br label %7

7:                                                ; preds = %13, %4
  %.0 = phi i32 [ 0, %4 ], [ %14, %13 ]
  %8 = icmp slt i32 %.0, %1
  br i1 %8, label %9, label %15

9:                                                ; preds = %7
  %10 = add nsw i32 %6, %.0
  %11 = sext i32 %.0 to i64
  %12 = getelementptr inbounds i32, ptr %0, i64 %11
  store i32 %10, ptr %12, align 4
  br label %13

13:                                               ; preds = %9
  %14 = add nsw i32 %.0, 1
  br label %7, !llvm.loop !6

15:                                               ; preds = %7
  ret void
}

; Function Attrs: noinline nounwind uwtable
define dso_local void @test_multiple_invariants(ptr noundef %0, i32 noundef %1, i32 noundef %2, i32 noundef %3, i32 noundef %4) #0 {
  %6 = add nsw i32 %2, %3
  %7 = mul nsw i32 %6, %4
  %8 = sub nsw i32 %7, 3
  br label %9

9:                                                ; preds = %15, %5
  %.0 = phi i32 [ 0, %5 ], [ %16, %15 ]
  %10 = icmp slt i32 %.0, %1
  br i1 %10, label %11, label %17

11:                                               ; preds = %9
  %12 = add nsw i32 %8, %.0
  %13 = sext i32 %.0 to i64
  %14 = getelementptr inbounds i32, ptr %0, i64 %13
  store i32 %12, ptr %14, align 4
  br label %15

15:                                               ; preds = %11
  %16 = add nsw i32 %.0, 1
  br label %9, !llvm.loop !8

17:                                               ; preds = %9
  ret void
}

; Function Attrs: noinline nounwind uwtable
define dso_local void @test_not_invariant(ptr noundef %0, i32 noundef %1, i32 noundef %2) #0 {
  br label %4

4:                                                ; preds = %11, %3
  %.0 = phi i32 [ 0, %3 ], [ %12, %11 ]
  %5 = icmp slt i32 %.0, %1
  br i1 %5, label %6, label %13

6:                                                ; preds = %4
  %7 = add nsw i32 %.0, %2
  %8 = mul nsw i32 %7, 2
  %9 = sext i32 %.0 to i64
  %10 = getelementptr inbounds i32, ptr %0, i64 %9
  store i32 %8, ptr %10, align 4
  br label %11

11:                                               ; preds = %6
  %12 = add nsw i32 %.0, 1
  br label %4, !llvm.loop !9

13:                                               ; preds = %4
  ret void
}

; Function Attrs: noinline nounwind uwtable
define dso_local void @test_nested_loops(ptr noundef %0, i32 noundef %1, i32 noundef %2, i32 noundef %3, i32 noundef %4) #0 {
  %6 = add nsw i32 %3, %4
  %7 = mul nsw i32 %3, %4
  br label %8

8:                                                ; preds = %23, %5
  %.01 = phi i32 [ 0, %5 ], [ %24, %23 ]
  %9 = icmp slt i32 %.01, %1
  br i1 %9, label %10, label %25

10:                                               ; preds = %8
  %11 = mul nsw i32 %.01, %2
  br label %12

12:                                               ; preds = %20, %10
  %.0 = phi i32 [ 0, %10 ], [ %21, %20 ]
  %13 = icmp slt i32 %.0, %2
  br i1 %13, label %14, label %22

14:                                               ; preds = %12
  %15 = add nsw i32 %7, %.0
  %16 = add nsw i32 %6, %15
  %17 = add nsw i32 %11, %.0
  %18 = sext i32 %17 to i64
  %19 = getelementptr inbounds i32, ptr %0, i64 %18
  store i32 %16, ptr %19, align 4
  br label %20

20:                                               ; preds = %14
  %21 = add nsw i32 %.0, 1
  br label %12, !llvm.loop !10

22:                                               ; preds = %12
  br label %23

23:                                               ; preds = %22
  %24 = add nsw i32 %.01, 1
  br label %8, !llvm.loop !11

25:                                               ; preds = %8
  ret void
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
!6 = distinct !{!6, !7}
!7 = !{!"llvm.loop.mustprogress"}
!8 = distinct !{!8, !7}
!9 = distinct !{!9, !7}
!10 = distinct !{!10, !7}
!11 = distinct !{!11, !7}
