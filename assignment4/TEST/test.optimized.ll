; ModuleID = 'test.m2r.ll'
source_filename = "test.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

@g_sink = dso_local global i32 0, align 4

; Function Attrs: noinline nounwind uwtable
define dso_local void @fuse_reset_then_bump(ptr noundef %0, i32 noundef %1) #0 {
  br label %3

3:                                                ; preds = %14, %2
  %.01 = phi i32 [ 0, %2 ], [ %15, %14 ]
  %4 = icmp slt i32 %.01, %1
  br i1 %4, label %5, label %16

5:                                                ; preds = %3
  %6 = sext i32 %.01 to i64
  %7 = getelementptr inbounds i32, ptr %0, i64 %6
  store i32 %.01, ptr %7, align 4
  %8 = sext i32 %.01 to i64
  %9 = getelementptr inbounds i32, ptr %0, i64 %8
  %10 = load i32, ptr %9, align 4
  %11 = add nsw i32 %10, 1
  %12 = sext i32 %.01 to i64
  %13 = getelementptr inbounds i32, ptr %0, i64 %12
  store i32 %11, ptr %13, align 4
  br label %14

14:                                               ; preds = %5
  %15 = add nsw i32 %.01, 1
  br label %3, !llvm.loop !6

16:                                               ; preds = %3
  ret void
}

; Function Attrs: noinline nounwind uwtable
define dso_local void @fuse_scale_then_shift(ptr noundef %0, i32 noundef %1) #0 {
  br label %3

3:                                                ; preds = %17, %2
  %.01 = phi i32 [ 0, %2 ], [ %18, %17 ]
  %4 = icmp slt i32 %.01, %1
  br i1 %4, label %5, label %19

5:                                                ; preds = %3
  %6 = mul nsw i32 %.01, 2
  %7 = sext i32 %.01 to i64
  %8 = getelementptr inbounds i32, ptr %0, i64 %7
  store i32 %6, ptr %8, align 4
  %9 = mul nsw i32 %.01, 17
  %10 = add nsw i32 %9, 5
  %11 = sext i32 %.01 to i64
  %12 = getelementptr inbounds i32, ptr %0, i64 %11
  %13 = load i32, ptr %12, align 4
  %14 = add nsw i32 %13, 3
  %15 = sext i32 %.01 to i64
  %16 = getelementptr inbounds i32, ptr %0, i64 %15
  store i32 %14, ptr %16, align 4
  br label %17

17:                                               ; preds = %5
  %18 = add nsw i32 %.01, 1
  br label %3, !llvm.loop !8

19:                                               ; preds = %3
  ret void
}

; Function Attrs: noinline nounwind uwtable
define dso_local void @no_fuse_different_trip_count(ptr noundef %0, i32 noundef %1) #0 {
  br label %3

3:                                                ; preds = %8, %2
  %.01 = phi i32 [ 0, %2 ], [ %9, %8 ]
  %4 = icmp slt i32 %.01, %1
  br i1 %4, label %5, label %10

5:                                                ; preds = %3
  %6 = sext i32 %.01 to i64
  %7 = getelementptr inbounds i32, ptr %0, i64 %6
  store i32 %.01, ptr %7, align 4
  br label %8

8:                                                ; preds = %5
  %9 = add nsw i32 %.01, 1
  br label %3, !llvm.loop !9

10:                                               ; preds = %3
  br label %11

11:                                               ; preds = %18, %10
  %.0 = phi i32 [ 0, %10 ], [ %19, %18 ]
  %12 = sub nsw i32 %1, 1
  %13 = icmp slt i32 %.0, %12
  br i1 %13, label %14, label %20

14:                                               ; preds = %11
  %15 = add nsw i32 %.0, 7
  %16 = sext i32 %.0 to i64
  %17 = getelementptr inbounds i32, ptr %0, i64 %16
  store i32 %15, ptr %17, align 4
  br label %18

18:                                               ; preds = %14
  %19 = add nsw i32 %.0, 1
  br label %11, !llvm.loop !10

20:                                               ; preds = %11
  ret void
}

; Function Attrs: noinline nounwind uwtable
define dso_local void @no_fuse_negative_distance(ptr noundef %0, i32 noundef %1) #0 {
  br label %3

3:                                                ; preds = %9, %2
  %.01 = phi i32 [ 0, %2 ], [ %10, %9 ]
  %4 = add nsw i32 %.01, 1
  %5 = icmp slt i32 %4, %1
  br i1 %5, label %6, label %11

6:                                                ; preds = %3
  %7 = sext i32 %.01 to i64
  %8 = getelementptr inbounds i32, ptr %0, i64 %7
  store i32 %.01, ptr %8, align 4
  br label %9

9:                                                ; preds = %6
  %10 = add nsw i32 %.01, 1
  br label %3, !llvm.loop !11

11:                                               ; preds = %3
  br label %12

12:                                               ; preds = %19, %11
  %.0 = phi i32 [ 0, %11 ], [ %20, %19 ]
  %13 = add nsw i32 %.0, 1
  %14 = icmp slt i32 %13, %1
  br i1 %14, label %15, label %21

15:                                               ; preds = %12
  %16 = add nsw i32 %.0, 1
  %17 = sext i32 %16 to i64
  %18 = getelementptr inbounds i32, ptr %0, i64 %17
  store i32 %.0, ptr %18, align 4
  br label %19

19:                                               ; preds = %15
  %20 = add nsw i32 %.0, 1
  br label %12, !llvm.loop !12

21:                                               ; preds = %12
  ret void
}

; Function Attrs: noinline nounwind uwtable
define dso_local void @no_fuse_gap_between_loops(ptr noundef %0, i32 noundef %1) #0 {
  br label %3

3:                                                ; preds = %8, %2
  %.01 = phi i32 [ 0, %2 ], [ %9, %8 ]
  %4 = icmp slt i32 %.01, %1
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
  %11 = load i32, ptr @g_sink, align 4
  %12 = add nsw i32 %11, %1
  store i32 %12, ptr @g_sink, align 4
  br label %13

13:                                               ; preds = %20, %10
  %.0 = phi i32 [ 0, %10 ], [ %21, %20 ]
  %14 = icmp slt i32 %.0, %1
  br i1 %14, label %15, label %22

15:                                               ; preds = %13
  %16 = load i32, ptr @g_sink, align 4
  %17 = add nsw i32 %.0, %16
  %18 = sext i32 %.0 to i64
  %19 = getelementptr inbounds i32, ptr %0, i64 %18
  store i32 %17, ptr %19, align 4
  br label %20

20:                                               ; preds = %15
  %21 = add nsw i32 %.0, 1
  br label %13, !llvm.loop !14

22:                                               ; preds = %13
  ret void
}

; Function Attrs: noinline nounwind uwtable
define dso_local void @no_fuse_branchy_body(ptr noundef %0, i32 noundef %1) #0 {
  br label %3

3:                                                ; preds = %22, %2
  %.01 = phi i32 [ 0, %2 ], [ %23, %22 ]
  %4 = icmp slt i32 %.01, %1
  br i1 %4, label %5, label %24

5:                                                ; preds = %3
  %6 = and i32 %.01, 1
  %7 = icmp eq i32 %6, 0
  br i1 %7, label %8, label %11

8:                                                ; preds = %5
  %9 = sext i32 %.01 to i64
  %10 = getelementptr inbounds i32, ptr %0, i64 %9
  store i32 %.01, ptr %10, align 4
  br label %15

11:                                               ; preds = %5
  %12 = sub nsw i32 0, %.01
  %13 = sext i32 %.01 to i64
  %14 = getelementptr inbounds i32, ptr %0, i64 %13
  store i32 %12, ptr %14, align 4
  br label %15

15:                                               ; preds = %11, %8
  %16 = sext i32 %.01 to i64
  %17 = getelementptr inbounds i32, ptr %0, i64 %16
  %18 = load i32, ptr %17, align 4
  %19 = add nsw i32 %18, 5
  %20 = sext i32 %.01 to i64
  %21 = getelementptr inbounds i32, ptr %0, i64 %20
  store i32 %19, ptr %21, align 4
  br label %22

22:                                               ; preds = %15
  %23 = add nsw i32 %.01, 1
  br label %3, !llvm.loop !15

24:                                               ; preds = %3
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
