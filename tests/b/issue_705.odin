package main

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
}