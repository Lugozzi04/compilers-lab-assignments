; ModuleID = 'test.O0.ll'
source_filename = "test.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

@g_sink = dso_local global i32 0, align 4

; Function Attrs: noinline nounwind uwtable
define dso_local void @fuse_reset_then_bump(ptr noundef %0) #0 {
  br label %2

2:                                                ; preds = %7, %1
  %.01 = phi i32 [ 0, %1 ], [ %8, %7 ]
  %3 = icmp slt i32 %.01, 8
  br i1 %3, label %4, label %9

4:                                                ; preds = %2
  %5 = sext i32 %.01 to i64
  %6 = getelementptr inbounds i32, ptr %0, i64 %5
  store i32 %.01, ptr %6, align 4
  br label %7

7:                                                ; preds = %4
  %8 = add nsw i32 %.01, 1
  br label %2, !llvm.loop !6

9:                                                ; preds = %2
  br label %10

10:                                               ; preds = %19, %9
  %.0 = phi i32 [ 0, %9 ], [ %20, %19 ]
  %11 = icmp slt i32 %.0, 8
  br i1 %11, label %12, label %21

12:                                               ; preds = %10
  %13 = sext i32 %.0 to i64
  %14 = getelementptr inbounds i32, ptr %0, i64 %13
  %15 = load i32, ptr %14, align 4
  %16 = add nsw i32 %15, 1
  %17 = sext i32 %.0 to i64
  %18 = getelementptr inbounds i32, ptr %0, i64 %17
  store i32 %16, ptr %18, align 4
  br label %19

19:                                               ; preds = %12
  %20 = add nsw i32 %.0, 1
  br label %10, !llvm.loop !8

21:                                               ; preds = %10
  ret void
}

; Function Attrs: noinline nounwind uwtable
define dso_local void @fuse_scale_then_shift(ptr noundef %0) #0 {
  br label %2

2:                                                ; preds = %8, %1
  %.01 = phi i32 [ 0, %1 ], [ %9, %8 ]
  %3 = icmp slt i32 %.01, 8
  br i1 %3, label %4, label %10

4:                                                ; preds = %2
  %5 = mul nsw i32 %.01, 2
  %6 = sext i32 %.01 to i64
  %7 = getelementptr inbounds i32, ptr %0, i64 %6
  store i32 %5, ptr %7, align 4
  br label %8

8:                                                ; preds = %4
  %9 = add nsw i32 %.01, 1
  br label %2, !llvm.loop !9

10:                                               ; preds = %2
  br label %11

11:                                               ; preds = %22, %10
  %.0 = phi i32 [ 0, %10 ], [ %23, %22 ]
  %12 = icmp slt i32 %.0, 8
  br i1 %12, label %13, label %24

13:                                               ; preds = %11
  %14 = mul nsw i32 %.0, 17
  %15 = add nsw i32 %14, 5
  %16 = sext i32 %.0 to i64
  %17 = getelementptr inbounds i32, ptr %0, i64 %16
  %18 = load i32, ptr %17, align 4
  %19 = add nsw i32 %18, 3
  %20 = sext i32 %.0 to i64
  %21 = getelementptr inbounds i32, ptr %0, i64 %20
  store i32 %19, ptr %21, align 4
  br label %22

22:                                               ; preds = %13
  %23 = add nsw i32 %.0, 1
  br label %11, !llvm.loop !10

24:                                               ; preds = %11
  ret void
}

; Function Attrs: noinline nounwind uwtable
define dso_local void @no_fuse_different_trip_count(ptr noundef %0) #0 {
  br label %2

2:                                                ; preds = %7, %1
  %.01 = phi i32 [ 0, %1 ], [ %8, %7 ]
  %3 = icmp slt i32 %.01, 8
  br i1 %3, label %4, label %9

4:                                                ; preds = %2
  %5 = sext i32 %.01 to i64
  %6 = getelementptr inbounds i32, ptr %0, i64 %5
  store i32 %.01, ptr %6, align 4
  br label %7

7:                                                ; preds = %4
  %8 = add nsw i32 %.01, 1
  br label %2, !llvm.loop !11

9:                                                ; preds = %2
  br label %10

10:                                               ; preds = %16, %9
  %.0 = phi i32 [ 0, %9 ], [ %17, %16 ]
  %11 = icmp slt i32 %.0, 7
  br i1 %11, label %12, label %18

12:                                               ; preds = %10
  %13 = add nsw i32 %.0, 7
  %14 = sext i32 %.0 to i64
  %15 = getelementptr inbounds i32, ptr %0, i64 %14
  store i32 %13, ptr %15, align 4
  br label %16

16:                                               ; preds = %12
  %17 = add nsw i32 %.0, 1
  br label %10, !llvm.loop !12

18:                                               ; preds = %10
  ret void
}

; Function Attrs: noinline nounwind uwtable
define dso_local void @no_fuse_negative_distance(ptr noundef %0, ptr noundef %1) #0 {
  br label %3

3:                                                ; preds = %8, %2
  %.01 = phi i32 [ 0, %2 ], [ %9, %8 ]
  %4 = icmp slt i32 %.01, 5
  br i1 %4, label %5, label %10

5:                                                ; preds = %3
  %6 = sext i32 %.01 to i64
  %7 = getelementptr inbounds i32, ptr %0, i64 %6
  store i32 %.01, ptr %7, align 4
  br label %8

8:                                                ; preds = %5
  %9 = add nsw i32 %.01, 1
  br label %3, !llvm.loop !13

10:                                               ; preds = %3
  br label %11

11:                                               ; preds = %20, %10
  %.0 = phi i32 [ 0, %10 ], [ %21, %20 ]
  %12 = icmp slt i32 %.0, 5
  br i1 %12, label %13, label %22

13:                                               ; preds = %11
  %14 = add nsw i32 %.0, 3
  %15 = sext i32 %14 to i64
  %16 = getelementptr inbounds i32, ptr %0, i64 %15
  %17 = load i32, ptr %16, align 4
  %18 = sext i32 %.0 to i64
  %19 = getelementptr inbounds i32, ptr %1, i64 %18
  store i32 %17, ptr %19, align 4
  br label %20

20:                                               ; preds = %13
  %21 = add nsw i32 %.0, 1
  br label %11, !llvm.loop !14

22:                                               ; preds = %11
  ret void
}

; Function Attrs: noinline nounwind uwtable
define dso_local void @no_fuse_gap_between_loops(ptr noundef %0) #0 {
  br label %2

2:                                                ; preds = %7, %1
  %.01 = phi i32 [ 0, %1 ], [ %8, %7 ]
  %3 = icmp slt i32 %.01, 8
  br i1 %3, label %4, label %9

4:                                                ; preds = %2
  %5 = sext i32 %.01 to i64
  %6 = getelementptr inbounds i32, ptr %0, i64 %5
  store i32 %.01, ptr %6, align 4
  br label %7

7:                                                ; preds = %4
  %8 = add nsw i32 %.01, 1
  br label %2, !llvm.loop !15

9:                                                ; preds = %2
  %10 = load i32, ptr @g_sink, align 4
  %11 = add nsw i32 %10, 8
  store i32 %11, ptr @g_sink, align 4
  br label %12

12:                                               ; preds = %19, %9
  %.0 = phi i32 [ 0, %9 ], [ %20, %19 ]
  %13 = icmp slt i32 %.0, 8
  br i1 %13, label %14, label %21

14:                                               ; preds = %12
  %15 = load i32, ptr @g_sink, align 4
  %16 = add nsw i32 %.0, %15
  %17 = sext i32 %.0 to i64
  %18 = getelementptr inbounds i32, ptr %0, i64 %17
  store i32 %16, ptr %18, align 4
  br label %19

19:                                               ; preds = %14
  %20 = add nsw i32 %.0, 1
  br label %12, !llvm.loop !16

21:                                               ; preds = %12
  ret void
}

; Function Attrs: noinline nounwind uwtable
define dso_local void @fuse_branchy_body(ptr noundef %0) #0 {
  br label %2

2:                                                ; preds = %15, %1
  %.01 = phi i32 [ 0, %1 ], [ %16, %15 ]
  %3 = icmp slt i32 %.01, 8
  br i1 %3, label %4, label %17

4:                                                ; preds = %2
  %5 = and i32 %.01, 1
  %6 = icmp eq i32 %5, 0
  br i1 %6, label %7, label %10

7:                                                ; preds = %4
  %8 = sext i32 %.01 to i64
  %9 = getelementptr inbounds i32, ptr %0, i64 %8
  store i32 %.01, ptr %9, align 4
  br label %14

10:                                               ; preds = %4
  %11 = sub nsw i32 0, %.01
  %12 = sext i32 %.01 to i64
  %13 = getelementptr inbounds i32, ptr %0, i64 %12
  store i32 %11, ptr %13, align 4
  br label %14

14:                                               ; preds = %10, %7
  br label %15

15:                                               ; preds = %14
  %16 = add nsw i32 %.01, 1
  br label %2, !llvm.loop !17

17:                                               ; preds = %2
  br label %18

18:                                               ; preds = %27, %17
  %.0 = phi i32 [ 0, %17 ], [ %28, %27 ]
  %19 = icmp slt i32 %.0, 8
  br i1 %19, label %20, label %29

20:                                               ; preds = %18
  %21 = sext i32 %.0 to i64
  %22 = getelementptr inbounds i32, ptr %0, i64 %21
  %23 = load i32, ptr %22, align 4
  %24 = add nsw i32 %23, 5
  %25 = sext i32 %.0 to i64
  %26 = getelementptr inbounds i32, ptr %0, i64 %25
  store i32 %24, ptr %26, align 4
  br label %27

27:                                               ; preds = %20
  %28 = add nsw i32 %.0, 1
  br label %18, !llvm.loop !18

29:                                               ; preds = %18
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
!17 = distinct !{!17, !7}
!18 = distinct !{!18, !7}
