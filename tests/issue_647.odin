package test
main :: proc() {
	a: i32;
	b: #relative(i8) ^i32 = &a;
	b^ = 1;
}
