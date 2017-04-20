; RUN: bc2allvm %s -o %t
; RUN: alley %t
; RUN: alltogether %t -o %t.unified
; RUN: alley %t.unified
define hidden i32 @main() {
	ret i32 0
}
