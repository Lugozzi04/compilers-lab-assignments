; ModuleID = 'test.c'
source_filename = "test.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; Function Attrs: noinline nounwind uwtable
define dso_local void @test_basic(ptr noundef %0, i32 noundef %1, i32 noundef %2, i32 noundef %3) #0 {
  %5 = alloca ptr, align 8
  %6 = alloca i32, align 4
  %7 = alloca i32, align 4
  %8 = alloca i32, align 4
  %9 = alloca i32, align 4
  %10 = alloca i32, align 4
  %11 = alloca i32, align 4
  store ptr %0, ptr %5, align 8
  store i32 %1, ptr %6, align 4
  store i32 %2, ptr %7, align 4
  store i32 %3, ptr %8, align 4
  store i32 0, ptr %9, align 4
  br label %12

12:                                               ; preds = %29, %4
  %13 = load i32, ptr %9, align 4
  %14 = load i32, ptr %6, align 4
  %15 = icmp slt i32 %13, %14
  br i1 %15, label %16, label %32

16:                                               ; preds = %12
  %17 = load i32, ptr %7, align 4
  %18 = load i32, ptr %8, align 4
  %19 = mul nsw i32 %17, %18
  store i32 %19, ptr %10, align 4
  %20 = load i32, ptr %10, align 4
  %21 = add nsw i32 %20, 10
  store i32 %21, ptr %11, align 4
  %22 = load i32, ptr %11, align 4
  %23 = load i32, ptr %9, align 4
  %24 = add nsw i32 %22, %23
  %25 = load ptr, ptr %5, align 8
  %26 = load i32, ptr %9, align 4
  %27 = sext i32 %26 to i64
  %28 = getelementptr inbounds i32, ptr %25, i64 %27
  store i32 %24, ptr %28, align 4
  br label %29

29:                                               ; preds = %16
  %30 = load i32, ptr %9, align 4
  %31 = add nsw i32 %30, 1
  store i32 %31, ptr %9, align 4
  br label %12, !llvm.loop !6

32:                                               ; preds = %12
  ret void
}

; Function Attrs: noinline nounwind uwtable
define dso_local void @test_multiple_invariants(ptr noundef %0, i32 noundef %1, i32 noundef %2, i32 noundef %3, i32 noundef %4) #0 {
  %6 = alloca ptr, align 8
  %7 = alloca i32, align 4
  %8 = alloca i32, align 4
  %9 = alloca i32, align 4
  %10 = alloca i32, align 4
  %11 = alloca i32, align 4
  %12 = alloca i32, align 4
  %13 = alloca i32, align 4
  %14 = alloca i32, align 4
  store ptr %0, ptr %6, align 8
  store i32 %1, ptr %7, align 4
  store i32 %2, ptr %8, align 4
  store i32 %3, ptr %9, align 4
  store i32 %4, ptr %10, align 4
  store i32 0, ptr %11, align 4
  br label %15

15:                                               ; preds = %35, %5
  %16 = load i32, ptr %11, align 4
  %17 = load i32, ptr %7, align 4
  %18 = icmp slt i32 %16, %17
  br i1 %18, label %19, label %38

19:                                               ; preds = %15
  %20 = load i32, ptr %8, align 4
  %21 = load i32, ptr %9, align 4
  %22 = add nsw i32 %20, %21
  store i32 %22, ptr %12, align 4
  %23 = load i32, ptr %12, align 4
  %24 = load i32, ptr %10, align 4
  %25 = mul nsw i32 %23, %24
  store i32 %25, ptr %13, align 4
  %26 = load i32, ptr %13, align 4
  %27 = sub nsw i32 %26, 3
  store i32 %27, ptr %14, align 4
  %28 = load i32, ptr %14, align 4
  %29 = load i32, ptr %11, align 4
  %30 = add nsw i32 %28, %29
  %31 = load ptr, ptr %6, align 8
  %32 = load i32, ptr %11, align 4
  %33 = sext i32 %32 to i64
  %34 = getelementptr inbounds i32, ptr %31, i64 %33
  store i32 %30, ptr %34, align 4
  br label %35

35:                                               ; preds = %19
  %36 = load i32, ptr %11, align 4
  %37 = add nsw i32 %36, 1
  store i32 %37, ptr %11, align 4
  br label %15, !llvm.loop !8

38:                                               ; preds = %15
  ret void
}

; Function Attrs: noinline nounwind uwtable
define dso_local void @test_not_invariant(ptr noundef %0, i32 noundef %1, i32 noundef %2) #0 {
  %4 = alloca ptr, align 8
  %5 = alloca i32, align 4
  %6 = alloca i32, align 4
  %7 = alloca i32, align 4
  %8 = alloca i32, align 4
  %9 = alloca i32, align 4
  store ptr %0, ptr %4, align 8
  store i32 %1, ptr %5, align 4
  store i32 %2, ptr %6, align 4
  store i32 0, ptr %7, align 4
  br label %10

10:                                               ; preds = %25, %3
  %11 = load i32, ptr %7, align 4
  %12 = load i32, ptr %5, align 4
  %13 = icmp slt i32 %11, %12
  br i1 %13, label %14, label %28

14:                                               ; preds = %10
  %15 = load i32, ptr %7, align 4
  %16 = load i32, ptr %6, align 4
  %17 = add nsw i32 %15, %16
  store i32 %17, ptr %8, align 4
  %18 = load i32, ptr %8, align 4
  %19 = mul nsw i32 %18, 2
  store i32 %19, ptr %9, align 4
  %20 = load i32, ptr %9, align 4
  %21 = load ptr, ptr %4, align 8
  %22 = load i32, ptr %7, align 4
  %23 = sext i32 %22 to i64
  %24 = getelementptr inbounds i32, ptr %21, i64 %23
  store i32 %20, ptr %24, align 4
  br label %25

25:                                               ; preds = %14
  %26 = load i32, ptr %7, align 4
  %27 = add nsw i32 %26, 1
  store i32 %27, ptr %7, align 4
  br label %10, !llvm.loop !9

28:                                               ; preds = %10
  ret void
}

; Function Attrs: noinline nounwind uwtable
define dso_local void @test_nested_loops(ptr noundef %0, i32 noundef %1, i32 noundef %2, i32 noundef %3, i32 noundef %4) #0 {
  %6 = alloca ptr, align 8
  %7 = alloca i32, align 4
  %8 = alloca i32, align 4
  %9 = alloca i32, align 4
  %10 = alloca i32, align 4
  %11 = alloca i32, align 4
  %12 = alloca i32, align 4
  %13 = alloca i32, align 4
  %14 = alloca i32, align 4
  %15 = alloca i32, align 4
  store ptr %0, ptr %6, align 8
  store i32 %1, ptr %7, align 4
  store i32 %2, ptr %8, align 4
  store i32 %3, ptr %9, align 4
  store i32 %4, ptr %10, align 4
  store i32 0, ptr %11, align 4
  br label %16

16:                                               ; preds = %50, %5
  %17 = load i32, ptr %11, align 4
  %18 = load i32, ptr %7, align 4
  %19 = icmp slt i32 %17, %18
  br i1 %19, label %20, label %53

20:                                               ; preds = %16
  %21 = load i32, ptr %9, align 4
  %22 = load i32, ptr %10, align 4
  %23 = add nsw i32 %21, %22
  store i32 %23, ptr %12, align 4
  store i32 0, ptr %13, align 4
  br label %24

24:                                               ; preds = %46, %20
  %25 = load i32, ptr %13, align 4
  %26 = load i32, ptr %8, align 4
  %27 = icmp slt i32 %25, %26
  br i1 %27, label %28, label %49

28:                                               ; preds = %24
  %29 = load i32, ptr %9, align 4
  %30 = load i32, ptr %10, align 4
  %31 = mul nsw i32 %29, %30
  store i32 %31, ptr %14, align 4
  %32 = load i32, ptr %14, align 4
  %33 = load i32, ptr %13, align 4
  %34 = add nsw i32 %32, %33
  store i32 %34, ptr %15, align 4
  %35 = load i32, ptr %12, align 4
  %36 = load i32, ptr %15, align 4
  %37 = add nsw i32 %35, %36
  %38 = load ptr, ptr %6, align 8
  %39 = load i32, ptr %11, align 4
  %40 = load i32, ptr %8, align 4
  %41 = mul nsw i32 %39, %40
  %42 = load i32, ptr %13, align 4
  %43 = add nsw i32 %41, %42
  %44 = sext i32 %43 to i64
  %45 = getelementptr inbounds i32, ptr %38, i64 %44
  store i32 %37, ptr %45, align 4
  br label %46

46:                                               ; preds = %28
  %47 = load i32, ptr %13, align 4
  %48 = add nsw i32 %47, 1
  store i32 %48, ptr %13, align 4
  br label %24, !llvm.loop !10

49:                                               ; preds = %24
  br label %50

50:                                               ; preds = %49
  %51 = load i32, ptr %11, align 4
  %52 = add nsw i32 %51, 1
  store i32 %52, ptr %11, align 4
  br label %16, !llvm.loop !11

53:                                               ; preds = %16
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
