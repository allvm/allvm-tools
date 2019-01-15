; RUN: llvm-as %s -o %t.bc
; RUN: bc2allvm %t.bc -o %t

; Previously this checked that symbol resolution errors
; (at least from main)
; produced errors even when running with -noexec.

; This is not intrinsically desirable,
; but at one point this behavior was relied upon
; for quick checking for incomplete allexe's.

; Newer LLVM versions cause this to pass,
; which for now seems the preferred behavior anyway.

; RUN: not alley %t |& FileCheck %s
; RUNX: not alley -noexec %t |& FileCheck %s
; RUN: alley -noexec %t

; CHECK: Program used external function

declare i32 @foo()

define i32 @main() {
	%val = call i32 @foo()
	ret i32 %val
}
