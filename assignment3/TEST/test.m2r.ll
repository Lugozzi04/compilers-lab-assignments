; ModuleID = 'test.O0.ll'
source_filename = "test.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; Function Attrs: noinline nounwind uwtable
define dso_local void @test_basic(ptr noundef %0, i32 noundef %1, i32 noundef %2, i32 noundef %3) #0 {
  br label %5

5:                                                ; preds = %13, %4
  %.0 = phi i32 [ 0, %4 ], [ %14, %13 ]
  %6 = icmp slt i32 %.0, %1
  br i1 %6, label %7, label %15

7:                                                ; preds = %5
  %8 = mul nsw i32 %2, %3
  %9 = add nsw i32 %8, 10
  %10 = add nsw i32 %9, %.0
  %11 = sext i32 %.0 to i64
  %12 = getelementptr inbounds i32, ptr %0, i64 %11
  store i32 %10, ptr %12, align 4
  br label %13

13:                                               ; preds = %7
  %14 = add nsw i32 %.0, 1
  br label %5, !llvm.loop !6

15:                                               ; preds = %5
  ret void
}

; Function Attrs: noinline nounwind uwtable
define dso_local void @test_multiple_invariants(ptr noundef %0, i32 noundef %1, i32 noundef %2, i32 noundef %3, i32 noundef %4) #0 {
  br label %6

6:                                                ; preds = %15, %5
  %.0 = phi i32 [ 0, %5 ], [ %16, %15 ]
  %7 = icmp slt i32 %.0, %1
  br i1 %7, label %8, label %17

8:                                                ; preds = %6
  %9 = add nsw i32 %2, %3
  %10 = mul nsw i32 %9, %4
  %11 = sub nsw i32 %10, 3
  %12 = add nsw i32 %11, %.0
  %13 = sext i32 %.0 to i64
  %14 = getelementptr inbounds i32, ptr %0, i64 %13
  store i32 %12, ptr %14, align 4
  br label %15

15:                                               ; preds = %8
  %16 = add nsw i32 %.0, 1
  br label %6, !llvm.loop !8

17:                                               ; preds = %6
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
  br label %6

6:                                                ; preds = %23, %5
  %.01 = phi i32 [ 0, %5 ], [ %24, %23 ]
  %7 = icmp slt i32 %.01, %1
  br i1 %7, label %8, label %25

8:                                                ; preds = %6
  %9 = add nsw i32 %3, %4
  br label %10

10:                                               ; preds = %20, %8
  %.0 = phi i32 [ 0, %8 ], [ %21, %20 ]
  %11 = icmp slt i32 %.0, %2
  br i1 %11, label %12, label %22

12:                                               ; preds = %10
  %13 = mul nsw i32 %3, %4
  %14 = add nsw i32 %13, %.0
  %15 = add nsw i32 %9, %14
  %16 = mul nsw i32 %.01, %2
  %17 = add nsw i32 %16, %.0
  %18 = sext i32 %17 to i64
  %19 = getelementptr inbounds i32, ptr %0, i64 %18
  store i32 %15, ptr %19, align 4
  br label %20

20:                                               ; preds = %12
  %21 = add nsw i32 %.0, 1
  br label %10, !llvm.loop !10

22:                                               ; preds = %10
  br label %23

23:                                               ; preds = %22
  %24 = add nsw i32 %.01, 1
  br label %6, !llvm.loop !11

25:                                               ; preds = %6
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
