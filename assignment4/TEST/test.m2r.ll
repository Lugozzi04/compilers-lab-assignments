; ModuleID = 'test.O0.ll'
source_filename = "test.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; Function Attrs: noinline nounwind uwtable
define dso_local void @guarded_raw(ptr noalias noundef %0, ptr noalias noundef %1, i32 noundef %2) #0 {
  %4 = icmp sgt i32 %2, 0
  br i1 %4, label %5, label %26

5:                                                ; preds = %3
  br label %6

6:                                                ; preds = %11, %5
  %.01 = phi i32 [ 0, %5 ], [ %12, %11 ]
  %7 = icmp slt i32 %.01, %2
  br i1 %7, label %8, label %13

8:                                                ; preds = %6
  %9 = sext i32 %.01 to i64
  %10 = getelementptr inbounds i32, ptr %0, i64 %9
  store i32 %.01, ptr %10, align 4
  br label %11

11:                                               ; preds = %8
  %12 = add nsw i32 %.01, 1
  br label %6, !llvm.loop !6

13:                                               ; preds = %6
  br label %14

14:                                               ; preds = %23, %13
  %.0 = phi i32 [ 0, %13 ], [ %24, %23 ]
  %15 = icmp slt i32 %.0, %2
  br i1 %15, label %16, label %25

16:                                               ; preds = %14
  %17 = sext i32 %.0 to i64
  %18 = getelementptr inbounds i32, ptr %0, i64 %17
  %19 = load i32, ptr %18, align 4
  %20 = add nsw i32 %19, 1
  %21 = sext i32 %.0 to i64
  %22 = getelementptr inbounds i32, ptr %1, i64 %21
  store i32 %20, ptr %22, align 4
  br label %23

23:                                               ; preds = %16
  %24 = add nsw i32 %.0, 1
  br label %14, !llvm.loop !8

25:                                               ; preds = %14
  br label %26

26:                                               ; preds = %25, %3
  ret void
}

; Function Attrs: noinline nounwind uwtable
define dso_local void @unguarded_raw(ptr noalias noundef %0, ptr noalias noundef %1, i32 noundef %2) #0 {
  br label %4

4:                                                ; preds = %9, %3
  %.01 = phi i32 [ 0, %3 ], [ %10, %9 ]
  %5 = icmp slt i32 %.01, %2
  br i1 %5, label %6, label %11

6:                                                ; preds = %4
  %7 = sext i32 %.01 to i64
  %8 = getelementptr inbounds i32, ptr %0, i64 %7
  store i32 %.01, ptr %8, align 4
  br label %9

9:                                                ; preds = %6
  %10 = add nsw i32 %.01, 1
  br label %4, !llvm.loop !9

11:                                               ; preds = %4
  br label %12

12:                                               ; preds = %21, %11
  %.0 = phi i32 [ 0, %11 ], [ %22, %21 ]
  %13 = icmp slt i32 %.0, %2
  br i1 %13, label %14, label %23

14:                                               ; preds = %12
  %15 = sext i32 %.0 to i64
  %16 = getelementptr inbounds i32, ptr %0, i64 %15
  %17 = load i32, ptr %16, align 4
  %18 = add nsw i32 %17, 1
  %19 = sext i32 %.0 to i64
  %20 = getelementptr inbounds i32, ptr %1, i64 %19
  store i32 %18, ptr %20, align 4
  br label %21

21:                                               ; preds = %14
  %22 = add nsw i32 %.0, 1
  br label %12, !llvm.loop !10

23:                                               ; preds = %12
  ret void
}

; Function Attrs: noinline nounwind uwtable
define dso_local i32 @guarded_rar_only(ptr noalias noundef %0, i32 noundef %1) #0 {
  %3 = icmp sgt i32 %1, 0
  br i1 %3, label %4, label %25

4:                                                ; preds = %2
  br label %5

5:                                                ; preds = %12, %4
  %.02 = phi i32 [ 0, %4 ], [ %11, %12 ]
  %.01 = phi i32 [ 0, %4 ], [ %13, %12 ]
  %6 = icmp slt i32 %.01, %1
  br i1 %6, label %7, label %14

7:                                                ; preds = %5
  %8 = sext i32 %.01 to i64
  %9 = getelementptr inbounds i32, ptr %0, i64 %8
  %10 = load i32, ptr %9, align 4
  %11 = add nsw i32 %.02, %10
  br label %12

12:                                               ; preds = %7
  %13 = add nsw i32 %.01, 1
  br label %5, !llvm.loop !11

14:                                               ; preds = %5
  br label %15

15:                                               ; preds = %22, %14
  %.1 = phi i32 [ %.02, %14 ], [ %21, %22 ]
  %.0 = phi i32 [ 0, %14 ], [ %23, %22 ]
  %16 = icmp slt i32 %.0, %1
  br i1 %16, label %17, label %24

17:                                               ; preds = %15
  %18 = sext i32 %.0 to i64
  %19 = getelementptr inbounds i32, ptr %0, i64 %18
  %20 = load i32, ptr %19, align 4
  %21 = add nsw i32 %.1, %20
  br label %22

22:                                               ; preds = %17
  %23 = add nsw i32 %.0, 1
  br label %15, !llvm.loop !12

24:                                               ; preds = %15
  br label %25

25:                                               ; preds = %24, %2
  %.2 = phi i32 [ %.1, %24 ], [ 0, %2 ]
  ret i32 %.2
}

; Function Attrs: noinline nounwind uwtable
define dso_local void @unguarded_no_dependence(ptr noalias noundef %0, ptr noalias noundef %1, i32 noundef %2) #0 {
  br label %4

4:                                                ; preds = %9, %3
  %.01 = phi i32 [ 0, %3 ], [ %10, %9 ]
  %5 = icmp slt i32 %.01, %2
  br i1 %5, label %6, label %11

6:                                                ; preds = %4
  %7 = sext i32 %.01 to i64
  %8 = getelementptr inbounds i32, ptr %0, i64 %7
  store i32 %.01, ptr %8, align 4
  br label %9

9:                                                ; preds = %6
  %10 = add nsw i32 %.01, 1
  br label %4, !llvm.loop !13

11:                                               ; preds = %4
  br label %12

12:                                               ; preds = %18, %11
  %.0 = phi i32 [ 0, %11 ], [ %19, %18 ]
  %13 = icmp slt i32 %.0, %2
  br i1 %13, label %14, label %20

14:                                               ; preds = %12
  %15 = add nsw i32 %.0, 1
  %16 = sext i32 %.0 to i64
  %17 = getelementptr inbounds i32, ptr %1, i64 %16
  store i32 %15, ptr %17, align 4
  br label %18

18:                                               ; preds = %14
  %19 = add nsw i32 %.0, 1
  br label %12, !llvm.loop !14

20:                                               ; preds = %12
  ret void
}

; Function Attrs: noinline nounwind uwtable
define dso_local void @guarded_negative_dependence(ptr noalias noundef %0, ptr noalias noundef %1, i32 noundef %2) #0 {
  %4 = icmp sgt i32 %2, 1
  br i1 %4, label %5, label %29

5:                                                ; preds = %3
  br label %6

6:                                                ; preds = %12, %5
  %.01 = phi i32 [ 0, %5 ], [ %13, %12 ]
  %7 = sub nsw i32 %2, 1
  %8 = icmp slt i32 %.01, %7
  br i1 %8, label %9, label %14

9:                                                ; preds = %6
  %10 = sext i32 %.01 to i64
  %11 = getelementptr inbounds i32, ptr %0, i64 %10
  store i32 %.01, ptr %11, align 4
  br label %12

12:                                               ; preds = %9
  %13 = add nsw i32 %.01, 1
  br label %6, !llvm.loop !15

14:                                               ; preds = %6
  br label %15

15:                                               ; preds = %26, %14
  %.0 = phi i32 [ 0, %14 ], [ %27, %26 ]
  %16 = sub nsw i32 %2, 1
  %17 = icmp slt i32 %.0, %16
  br i1 %17, label %18, label %28

18:                                               ; preds = %15
  %19 = add nsw i32 %.0, 1
  %20 = sext i32 %19 to i64
  %21 = getelementptr inbounds i32, ptr %0, i64 %20
  %22 = load i32, ptr %21, align 4
  %23 = add nsw i32 %22, 1
  %24 = sext i32 %.0 to i64
  %25 = getelementptr inbounds i32, ptr %1, i64 %24
  store i32 %23, ptr %25, align 4
  br label %26

26:                                               ; preds = %18
  %27 = add nsw i32 %.0, 1
  br label %15, !llvm.loop !16

28:                                               ; preds = %15
  br label %29

29:                                               ; preds = %28, %3
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
!12 = distinct !{!12, !7}
!13 = distinct !{!13, !7}
!14 = distinct !{!14, !7}
!15 = distinct !{!15, !7}
!16 = distinct !{!16, !7}
