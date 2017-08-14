; Check allmux doesn't break on bitcast'd ctors
; RUN: rm -rf %t && mkdir -p %t/mux
; RUN: bc2allvm %s -o %t/test
; RUN: alley %t/test |& FileCheck %s
; Single-allexe mux:
; RUN: allmux %t/test -o %t/mux/test
; RUN: alley %t/mux/test |& FileCheck %s

; CHECK: Foo
; CHECK: Bar

; ModuleID = 'allmux-static-ctor-bitcast.cpp'
source_filename = "allmux-static-ctor-bitcast.cpp"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-musl"

@llvm.global_ctors = appending global [1 x { i32, void ()*, i8* }] [{ i32, void ()*, i8* } { i32 65535, void ()* bitcast (i32 ()* @_Z3foov to void ()*), i8* null }]
@str = private unnamed_addr constant [5 x i8] c"Foo!\00"
@str.2 = private unnamed_addr constant [5 x i8] c"Bar!\00"

; Function Attrs: nounwind uwtable
define i32 @_Z3foov() #0 {
entry:
  %puts = tail call i32 @puts(i8* getelementptr inbounds ([5 x i8], [5 x i8]* @str, i64 0, i64 0))
  ret i32 5
}

; Function Attrs: norecurse nounwind uwtable
define i32 @main() local_unnamed_addr #1 {
entry:
  %puts = tail call i32 @puts(i8* getelementptr inbounds ([5 x i8], [5 x i8]* @str.2, i64 0, i64 0))
  ret i32 0
}

; Function Attrs: nounwind
declare i32 @puts(i8* nocapture readonly) #2

attributes #0 = { nounwind uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { norecurse nounwind uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { nounwind }

!llvm.ident = !{!0}

!0 = !{!"clang version 4.0.1 (tags/RELEASE_401/final)"}
