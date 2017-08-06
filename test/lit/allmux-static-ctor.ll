; Check allmux'ing allexe's w/static constructors

; RUN: rm -rf %t && mkdir -p %t/mux %t/mux2 %t/mux3
; RUN: bc2allvm %s -o %t/test
; RUN: alley %t/test |& FileCheck %s
; Single-allexe mux:
; RUN: allmux %t/test -o %t/mux/test
; RUN: alley %t/mux/test |& FileCheck %s

; multiple-allexe mux
; RUN: cp %t/test %t/test2
; RUN: allmux %t/test %t/test2 -o %t/mux2/mux
; RUN: ln -s %t/mux2/mux %t/mux2/test
; RUN: ln -s %t/mux2/mux %t/mux2/test2

; RUN: alley %t/mux2/test |& FileCheck %s
; RUN: alley %t/mux2/test2 |& FileCheck %s

; Test behavior affter alltogether/allready
; RUN: alltogether %t/mux2/mux -o %t/mux3/mux
; RUN: allready %t/mux3/mux
; RUN: ln -s %t/mux3/mux %t/mux3/test
; RUN: ln -s %t/mux3/mux %t/mux3/test2
; RUN: alley -force-static %t/mux3/test |& FileCheck %s
; RUN: alley -force-static %t/mux3/test2 |& FileCheck %s

; CHECK: Hi
; CHECK-NOT: Hi
; CHECK: val=0
; CHECK-NOT: Hi

; ModuleID = 'static_ctor.bc'
source_filename = "static_ctor.cpp"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-musl"

@.str.1 = private unnamed_addr constant [8 x i8] c"val=%d\0A\00", align 1
@llvm.global_ctors = appending global [1 x { i32, void ()*, i8* }] [{ i32, void ()*, i8* } { i32 65535, void ()* @_GLOBAL__sub_I_static_ctor.cpp, i8* null }]
@str = private unnamed_addr constant [4 x i8] c"Hi!\00"

; Function Attrs: nounwind uwtable
define i32 @_Z13print_and_retv() local_unnamed_addr #0 {
entry:
  %puts = tail call i32 @puts(i8* getelementptr inbounds ([4 x i8], [4 x i8]* @str, i64 0, i64 0))
  ret i32 0
}

; Function Attrs: nounwind
declare i32 @printf(i8* nocapture readonly, ...) local_unnamed_addr #1

; Function Attrs: norecurse nounwind uwtable
define i32 @main() local_unnamed_addr #2 {
entry:
  %call = tail call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([8 x i8], [8 x i8]* @.str.1, i64 0, i64 0), i32 0)
  ret i32 0
}

; Function Attrs: nounwind uwtable
define internal void @_GLOBAL__sub_I_static_ctor.cpp() #0 section ".text.startup" {
entry:
  %puts.i.i = tail call i32 @puts(i8* getelementptr inbounds ([4 x i8], [4 x i8]* @str, i64 0, i64 0)) #3
  ret void
}

; Function Attrs: nounwind
declare i32 @puts(i8* nocapture readonly) #3

attributes #0 = { nounwind uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { nounwind "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { norecurse nounwind uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #3 = { nounwind }

!llvm.ident = !{!0}

!0 = !{!"clang version 4.0.0 "}
