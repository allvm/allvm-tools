; Convert "compiler barrier" idiom
; __asm__ __volatile__ ("" ::: "memory")
; Which can be replaced with C11 'atomic_signal_fence(memory_order_acq_rel)'
; In LLVM IR that becomes (at least in my sample program)
; 'fence singlethread acq_rel'

; https://en.wikipedia.org/wiki/Memory_ordering#Compile-time_memory_barrier_implementation
; http://en.cppreference.com/w/c/atomic/atomic_signal_fence

; Check that we can make an allexe out of this
; RUN: bc2allvm %s -f -o %t
; RUN: %t

; Check all asm has been removed:
; RUN: allopt -analyze -i %t llvm-dis |& FileCheck %s

; CHECK-NOT: call {{.*}} asm
; CHECK: fence singlethread acq_rel
; CHECK-NOT: call {{.*}} asm

; ModuleID = 'barrier.c'
source_filename = "barrier.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; Function Attrs: nounwind uwtable
define i32 @main() local_unnamed_addr #0 {
  tail call void asm sideeffect "", "~{memory},~{dirflag},~{fpsr},~{flags}"() #1, !srcloc !1
  ret i32 0
}

attributes #0 = { nounwind uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { nounwind }

!llvm.ident = !{!0}

!0 = !{!"clang version 4.0.0 (tags/RELEASE_400/final)"}
!1 = !{i32 -2147473092}
