package test

import "core:os"

Dummy :: struct {
	dummy: bool,	// crash:  src/array.cpp(14): Assertion Failure: `0 <= index && index < count` Index 2 is out of bounds ranges 0..<2
	// dummy: int,	// ok
}

foo :: proc (dummy: Dummy, args: []string) {
}

main :: proc() {
	dummy := Dummy{};
	foo(dummy, os.args);
}
