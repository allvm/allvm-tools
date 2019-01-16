; RUN: llvm-as %s -o %t.bc
; RUN: bc2allvm %t.bc -o %t


; RUN: not alley %t |& FileCheck %s
; RUN: not alley -noexec %t |& FileCheck %s

; CHECK: Program used external function

declare i32 @foo()

define i32 @main() {
	%val = call i32 @foo()
	ret i32 %val
}
