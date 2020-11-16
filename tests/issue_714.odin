package test
import "core:fmt"
main :: proc() {
	X :: bit_set[enum{A,B,C}];
	x: X = {.B};
	fmt.println(x);
}

