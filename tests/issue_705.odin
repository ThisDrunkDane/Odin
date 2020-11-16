package test

import "core:fmt"

Polygon :: struct {
	width, height: int,
	derived: any,
}

Triangle :: struct {
	using base: Polygon,
}

main :: proc() {
	triangle := new(Triangle);
	triangle.derived = triangle;
	triangle.width = 4;
	triangle.height = 5;

	assert(triangle.width == 4);

	// src/ir.cpp(6290): Panic: NOT FOUND ir_type_info_index u64 @ index 98

	// Comment out print to get rid of the panic.
	// fmt.println(triangle.width);
}
