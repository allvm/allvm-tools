; RUN: llvm-as %s -o %t.bc
; RUN: bc2allvm %t.bc -o %t
; RUN: ALLVM_CACHE_DIR=%t-cache allready %t
; RUN: ALLVM_CACHE_DIR=%t-jit alley %t 0000000000000000
; RUN: ALLVM_CACHE_DIR=%t-static alley -force-static %t 0000000000000000

; Calculates a 128-bit modulo, which on x86-64 becomes a call to the function
; __umodti3() in compiler-rt.

define i32 @main(i32, i8**) {
  %3 = getelementptr i8*, i8** %1, i64 1
  %4 = load i8*, i8** %3
  %5 = bitcast i8* %4 to i128*
  %6 = load i128, i128* %5
  %7 = urem i128 %6, 3
  %8 = trunc i128 %7 to i32
  ret i32 %8
}
