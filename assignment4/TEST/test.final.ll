; ModuleID = 'test.optimized.ll'
source_filename = "test.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

@g_sink = dso_local global i32 0, align 4

; Function Attrs: noinline nounwind uwtable
define dso_local void @fuse_reset_then_bump(ptr noundef %0) #0 {
  br label %2

2:                                                ; preds = %7, %1
  %.01 = phi i32 [ 0, %1 ], [ %14, %7 ]
  %3 = icmp slt i32 %.01, 8
  br i1 %3, label %4, label %21

4:                                                ; preds = %2
  %5 = sext i32 %.01 to i64
  %6 = getelementptr inbounds i32, ptr %0, i64 %5
  store i32 %.01, ptr %6, align 4
  br label %7

7:                                                ; preds = %4
  %8 = sext i32 %.01 to i64
  %9 = getelementptr inbounds i32, ptr %0, i64 %8
  %10 = load i32, ptr %9, align 4
  %11 = add nsw i32 %10, 1
  %12 = sext i32 %.01 to i64
  %13 = getelementptr inbounds i32, ptr %0, i64 %12
  store i32 %11, ptr %13, align 4
  %14 = add nsw i32 %.01, 1
  br label %2, !llvm.loop !6

15:                                               ; No predecessors!
  br label %16

16:                                               ; preds = %19, %15
  %.0 = phi i32 [ 0, %15 ], [ %20, %19 ]
  %17 = icmp slt i32 %.0, 8
  br i1 %17, label %18, label %21

18:                                               ; preds = %16
  br label %19

19:                                               ; preds = %18
  %20 = add nsw i32 %.0, 1
  br label %16, !llvm.loop !8

21:                                               ; preds = %16, %2
  ret void
}

; Function Attrs: noinline nounwind uwtable
define dso_local void @fuse_scale_then_shift(ptr noundef %0) #0 {
  br label %2

2:                                                ; preds = %8, %1
  %.01 = phi i32 [ 0, %1 ], [ %15, %8 ]
  %3 = icmp slt i32 %.01, 8
  br i1 %3, label %4, label %22

4:                                                ; preds = %2
  %5 = mul nsw i32 %.01, 2
  %6 = sext i32 %.01 to i64
  %7 = getelementptr inbounds i32, ptr %0, i64 %6
  store i32 %5, ptr %7, align 4
  br label %8

8:                                                ; preds = %4
  %9 = sext i32 %.01 to i64
  %10 = getelementptr inbounds i32, ptr %0, i64 %9
  %11 = load i32, ptr %10, align 4
  %12 = add nsw i32 %11, 3
  %13 = sext i32 %.01 to i64
  %14 = getelementptr inbounds i32, ptr %0, i64 %13
  store i32 %12, ptr %14, align 4
  %15 = add nsw i32 %.01, 1
  br label %2, !llvm.loop !9

16:                                               ; No predecessors!
  br label %17

17:                                               ; preds = %20, %16
  %.0 = phi i32 [ 0, %16 ], [ %21, %20 ]
  %18 = icmp slt i32 %.0, 8
  br i1 %18, label %19, label %22

19:                                               ; preds = %17
  br label %20

20:                                               ; preds = %19
  %21 = add nsw i32 %.0, 1
  br label %17, !llvm.loop !10

22:                                               ; preds = %17, %2
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
  %.01 = phi i32 [ 0, %1 ], [ %22, %15 ]
  %3 = icmp slt i32 %.01, 8
  br i1 %3, label %4, label %29

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
  %16 = sext i32 %.01 to i64
  %17 = getelementptr inbounds i32, ptr %0, i64 %16
  %18 = load i32, ptr %17, align 4
  %19 = add nsw i32 %18, 5
  %20 = sext i32 %.01 to i64
  %21 = getelementptr inbounds i32, ptr %0, i64 %20
  store i32 %19, ptr %21, align 4
  %22 = add nsw i32 %.01, 1
  br label %2, !llvm.loop !17

23:                                               ; No predecessors!
  br label %24

24:                                               ; preds = %27, %23
  %.0 = phi i32 [ 0, %23 ], [ %28, %27 ]
  %25 = icmp slt i32 %.0, 8
  br i1 %25, label %26, label %29

26:                                               ; preds = %24
  br label %27

27:                                               ; preds = %26
  %28 = add nsw i32 %.0, 1
  br label %24, !llvm.loop !18

29:                                               ; preds = %24, %2
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
