
declare i32 @foo()

define i32 @main() {
	%val = call i32 @foo()
	%diff = sub i32 %val, 5
	ret i32 %diff
}
