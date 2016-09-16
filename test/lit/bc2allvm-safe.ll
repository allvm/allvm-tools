; RUN: llvm-as %s -o %t.bc
; RUN: bc2allvm %t.bc -f -o %t
; RUN: not bc2allvm %t.bc -o %t
; RUN: bc2allvm %t.bc -f -o %t

define i32 @main() {
	ret i32 0;
}
