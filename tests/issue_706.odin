package test

import "core:fmt"

A :: struct #raw_union {
	bits : bit_field {
		mantissa : 23,
		exponent : 8,
		sign     : 1,
	},
	f : f32,
	u : u32,
};

B :: struct #raw_union {
	using _ : bit_field {
		mantissa : 23,
		exponent : 8,
		sign     : 1,
	},
	f : f32,
	u : u32,
};

main :: proc() {
	a: A;
	a.f = +0.0;
	assert(a.bits.sign == 0);
	assert(a.bits.exponent == 0);
	assert(a.bits.mantissa == 0);
	a.f = -0.0;
	assert(a.bits.sign == 1);
	assert(a.bits.exponent == 0);
	assert(a.bits.mantissa == 0);

	//

	b: B;
	b.f = +0.0;
	assert(b.sign == 0);
	assert(b.exponent == 0);
	assert(b.mantissa == 0);
	b.f = -0.0;
	assert(b.sign == 1);     // error: sign is 0 (should be 1)
	assert(b.exponent == 0);
	assert(b.mantissa == 0);
}
