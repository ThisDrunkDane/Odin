package test

import "core:fmt"
import "core:strings"

main :: proc() {
	i := i128(42);

	f := f64(i);
	g := i64(i);

	str := fmt.tprintln(i, f, g); // 42 0.000 42
	assert( strings.contains(str, "0.") == false );
}

