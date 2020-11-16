package test
main :: proc() {
	foo :: proc(value: ^$T, other_value := T(0)) {}
	val: f32;
	foo(&val);
}
