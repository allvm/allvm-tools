; RUN: llvm-as %s -o %t.bc
; RUN: bc2allvm %t.bc -o %t

; Check that symbol resolution errors (at least from main)
; produce errors even when running with -noexec.
; RUN: not alley %t |& FileCheck %s
; RUN: not alley -noexec %t |& FileCheck %s

; CHECK: Program used external function

declare i32 @foo()

define i32 @main() {
	%val = call i32 @foo()
	ret i32 %val
}
