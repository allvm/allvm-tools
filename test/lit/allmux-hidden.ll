; RUN: llvm-as %s -o %t.bc
; RUN: bc2allvm %t.bc -f -o %t
; RUN: alley %t

; RUN: rm -rf %t-{in,out} && mkdir -p %t-in %t-out
; RUN: ln -sf %t %t-in/test
; RUN: allmux %t-in/test -f -o %t-out/test-mux
; RUN: alltogether %t-out/test-mux -o %t-out/test
; RUN: allready %t-out/test
; RUN: alley %t-out/test

define hidden i32 @main() {
	ret i32 0;
}
