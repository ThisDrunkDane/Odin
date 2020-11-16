package test

Thing :: struct {
    a, b, c: f32,
}

main :: proc() {

    things: [dynamic]Thing;
    defer delete(things);

    a : f32 = 1;
    b : f32 = 2;
    c : f32 = 3;

    append(&things, Thing { a, b, c });
}
