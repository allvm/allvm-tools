; RUN: bc2allvm %s -f -o %t
; RUN: alley %t
; RUN: alltogether %t -f -o %t.unified
; RUN: alley %t.unified
define hidden i32 @main() {
	ret i32 0
}
