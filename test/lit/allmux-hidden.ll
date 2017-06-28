; RUN: llvm-as %s -o %t.bc
; RUN: bc2allvm %t.bc -o %t
; RUN: alley %t

; RUN: mkdir -p %t-{in,out}
; RUN: ln -sf %t %t-in/test
; RUN: allmux %t-in/test -o %t-out/test-mux
; RUN: alltogether %t-out/test-mux -o %t-out/test
; RUN: allready %t-out/test
; RUN: alley %t-out/test

define hidden i32 @main() {
	ret i32 0;
}
