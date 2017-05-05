; ModuleID = 'test/lit/deasm-bswap.c'
source_filename = "test/lit/deasm-bswap.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

@.str = private unnamed_addr constant [53 x i8] c"call_bswap(0x12345678) returned %lld, expected %lld\0A\00", align 1
@.str.1 = private unnamed_addr constant [54 x i8] c"call_bswapl(0x12345678) returned %lld, expected %lld\0A\00", align 1
@.str.2 = private unnamed_addr constant [65 x i8] c"call_bswapq(0xabcdef0012345678ull) returned %lld, expected %lld\0A\00", align 1
@.str.3 = private unnamed_addr constant [66 x i8] c"call_bswap64(0xabcdef0012345678ull) returned %lld, expected %lld\0A\00", align 1

; Function Attrs: nounwind readnone uwtable
define i32 @call_bswap(i32) local_unnamed_addr #0 {
  %2 = tail call i32 asm "bswap $0", "=r,r,~{dirflag},~{fpsr},~{flags}"(i32 %0) #3, !srcloc !1
  ret i32 %2
}

; Function Attrs: nounwind readnone uwtable
define i64 @call_bswap64(i64) local_unnamed_addr #0 {
  %2 = tail call i64 asm "bswap $0", "=r,r,~{dirflag},~{fpsr},~{flags}"(i64 %0) #3, !srcloc !2
  ret i64 %2
}

; Function Attrs: nounwind readnone uwtable
define i32 @call_bswapl(i32) local_unnamed_addr #0 {
  %2 = tail call i32 asm "bswapl $0", "=r,r,~{dirflag},~{fpsr},~{flags}"(i32 %0) #3, !srcloc !3
  ret i32 %2
}

; Function Attrs: nounwind readnone uwtable
define i64 @call_bswapq(i64) local_unnamed_addr #0 {
  %2 = tail call i64 asm "bswapq $0", "=r,r,~{dirflag},~{fpsr},~{flags}"(i64 %0) #3, !srcloc !4
  ret i64 %2
}

; Function Attrs: nounwind uwtable
define i32 @main() local_unnamed_addr #1 {
  %1 = tail call i32 asm "bswap $0", "=r,r,~{dirflag},~{fpsr},~{flags}"(i32 305419896) #3, !srcloc !1
  %2 = icmp eq i32 %1, 2018915346
  br i1 %2, label %6, label %3

; <label>:3:                                      ; preds = %0
  %4 = zext i32 %1 to i64
  %5 = tail call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([53 x i8], [53 x i8]* @.str, i64 0, i64 0), i64 %4, i64 2018915346)
  br label %22

; <label>:6:                                      ; preds = %0
  %7 = tail call i32 asm "bswapl $0", "=r,r,~{dirflag},~{fpsr},~{flags}"(i32 305419896) #3, !srcloc !3
  %8 = icmp eq i32 %7, 2018915346
  br i1 %8, label %12, label %9

; <label>:9:                                      ; preds = %6
  %10 = zext i32 %7 to i64
  %11 = tail call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([54 x i8], [54 x i8]* @.str.1, i64 0, i64 0), i64 %10, i64 2018915346)
  br label %22

; <label>:12:                                     ; preds = %6
  %13 = tail call i64 asm "bswapq $0", "=r,r,~{dirflag},~{fpsr},~{flags}"(i64 -6066930339413731720) #3, !srcloc !4
  %14 = icmp eq i64 %13, 8671175384478240171
  br i1 %14, label %17, label %15

; <label>:15:                                     ; preds = %12
  %16 = tail call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([65 x i8], [65 x i8]* @.str.2, i64 0, i64 0), i64 %13, i64 8671175384478240171)
  br label %22

; <label>:17:                                     ; preds = %12
  %18 = tail call i64 asm "bswap $0", "=r,r,~{dirflag},~{fpsr},~{flags}"(i64 -6066930339413731720) #3, !srcloc !2
  %19 = icmp eq i64 %18, 8671175384478240171
  br i1 %19, label %22, label %20

; <label>:20:                                     ; preds = %17
  %21 = tail call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([66 x i8], [66 x i8]* @.str.3, i64 0, i64 0), i64 %18, i64 8671175384478240171)
  br label %22

; <label>:22:                                     ; preds = %3, %9, %15, %17, %20
  %23 = phi i32 [ 1, %20 ], [ 0, %17 ], [ 1, %15 ], [ 1, %9 ], [ 1, %3 ]
  ret i32 %23
}

; Function Attrs: nounwind
declare i32 @printf(i8* nocapture readonly, ...) local_unnamed_addr #2

attributes #0 = { nounwind readnone uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { nounwind uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { nounwind "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #3 = { nounwind readnone }

!llvm.ident = !{!0}

!0 = !{!"clang version 4.0.0 (tags/RELEASE_400/final)"}
!1 = !{i32 159}
!2 = !{i32 281}
!3 = !{i32 402}
!4 = !{i32 524}
