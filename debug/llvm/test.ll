; ModuleID = 'test.c'
source_filename = "test.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

%struct.A = type { i32, i32 }

; Function Attrs: noinline nounwind optnone sspstrong uwtable
define dso_local i32 @main() #0 {
  %1 = alloca i32, align 4
  %2 = alloca %struct.A, align 4
  %3 = alloca %struct.A*, align 8
  %4 = alloca %struct.A*, align 8
  store i32 0, i32* %1, align 4
  store %struct.A* %2, %struct.A** %3, align 8
  %5 = load %struct.A*, %struct.A** %3, align 8
  %6 = getelementptr inbounds %struct.A, %struct.A* %5, i32 0, i32 0
  store i32 5, i32* %6, align 4
  store %struct.A* %2, %struct.A** %4, align 8
  %7 = load %struct.A*, %struct.A** %3, align 8
  %8 = getelementptr inbounds %struct.A, %struct.A* %7, i32 0, i32 1
  store i32 4, i32* %8, align 4
  ret i32 1
}

attributes #0 = { noinline nounwind optnone sspstrong uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }

!llvm.module.flags = !{!0, !1, !2, !3, !4}
!llvm.ident = !{!5}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 7, !"PIC Level", i32 2}
!2 = !{i32 7, !"PIE Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 1}
!4 = !{i32 7, !"frame-pointer", i32 2}
!5 = !{!"clang version 14.0.6"}
