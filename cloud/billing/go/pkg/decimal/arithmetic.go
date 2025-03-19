package decimal

import (
	"math/bits"
)

// Abs return |d| (the absolute value of d).
func (d Decimal128) Abs() Decimal128 {
	d.neg = false
	return d
}

// Neg return -d
func (d Decimal128) Neg() Decimal128 {
	if d.form != nanDecimal && !d.IsZero() {
		d.neg = !d.neg
	}
	return d
}

// Sign returns:
//
//	-1 if d <  0
//	 0 if d == 0
//	+1 if d >  0
//   0 if d is NaN
//
func (d Decimal128) Sign() int {
	switch {
	case d.IsZero() || d.IsNan():
		return 0
	case d.neg:
		return -1
	default:
		return 1
	}
}

// Signbit reports whether d is negative.
func (d Decimal128) Signbit() bool {
	return d.neg
}

// Cmp compares d and v and returns:
//
//   -1 if d <  v
//    0 if d == v
//   +1 if d >  v
//    0 if d is NaN or v is NaN
//
func (d Decimal128) Cmp(v Decimal128) int {
	switch { // check for special cases
	case d.form == finiteDecimal && v.form == finiteDecimal:
		break // if both numbers are finite then compare sings and values
	case d.form == nanDecimal || v.form == nanDecimal:
		// if one of numbers is NaN return 0
		return 0
	case d.form == infDecimal || v.form == infDecimal:
		switch {
		case d.neg != v.neg: // if signs are different then positive number is bigger
			if d.neg {
				return -1
			}
			return 1
		case d.form == v.form: // if both are Inf with same sign
			return 0
		case d.form == infDecimal: // d is inf
			if d.neg {
				return -1
			}
			return 1
		default: // v is inf
			if v.neg {
				return 1
			}
			return -1
		}
	}

	if d.neg != v.neg { // if signs are different then positive number is bigger
		if d.neg {
			return -1
		}
		return 1
	}
	// compare absolute values
	cmp := finiteAbsCmp(d.abs, v.abs)
	if d.neg {
		cmp = -cmp
	}
	return cmp
}

// Add returns result of d+v. If d is NaN or v is NaN then result is NaN.
func (d Decimal128) Add(v Decimal128) (result Decimal128) {
	switch { // special cases
	case nanOp(d, v): // if one of numbers is NaN
		return decimalNan
	case d.IsInf():
		return d
	case v.IsInf():
		return v
	}

	if d.neg != v.neg { // If signs are different then make sub -v
		v.neg = !v.neg
		return d.Sub(v)
	}
	result.neg = d.neg // addition with same signs can not change sign
	defer result.norm()

	// make double word addition with carry
	carry := uint64(0)
	result.abs[0], carry = bits.Add64(d.abs[0], v.abs[0], carry)
	result.abs[1], carry = bits.Add64(d.abs[1], v.abs[1], carry)
	if carry != 0 { // This case inaccessible with correct operands. But for sure keep this here.
		result.form = infDecimal
	}
	return
}

// Sub returns result of d-v. If d is NaN or v is NaN then result is NaN.
func (d Decimal128) Sub(v Decimal128) (result Decimal128) {
	switch { // special cases
	case nanOp(d, v): // if one of numbers is NaN
		return decimalNan
	case d.IsInf():
		return d
	case v.IsInf():
		return v.Neg()
	}

	if d.neg != v.neg { // If signs are different then make add -v
		v.neg = !v.neg
		return d.Add(v)
	}
	defer result.norm()

	switch finiteAbsCmp(d.abs, v.abs) { // define sign of result
	case 0:
		return
	case -1: // if d is less than v then result has revese sing and abs(result) = abs(v) - abs(d)
		result.neg = !d.neg
		d, v = v, d
	case 1: // if d is bigger than v then result has its sign
		result.neg = d.neg
	}

	// make double word addition with borrow
	borrow := uint64(0)
	result.abs[0], borrow = bits.Sub64(d.abs[0], v.abs[0], borrow)
	result.abs[1], borrow = bits.Sub64(d.abs[1], v.abs[1], borrow)

	if borrow != 0 { // This case inaccessible with correct operands. But for sure keep this here.
		result.form = infDecimal
	}
	return
}

// Mul returns result of d-v. If d is NaN or v is NaN then result is NaN.
func (d Decimal128) Mul(v Decimal128) (result Decimal128) {
	switch { // special cases
	case nanOp(d, v): // if one of numbers is NaN
		return decimalNan
	case d.IsInf():
		return d
	case v.IsInf():
		return v
	}

	if d.IsZero() || v.IsZero() { // zero multiplication cause zero result
		return
	}
	// 1*x = x
	if d == decimalOne {
		return v
	}
	if v == decimalOne {
		return d
	}

	defer result.norm()
	result.neg = d.neg != v.neg // result will be negative only if signs are different

	// if we simply multiply values than we get incorrectly scaled number:
	//   (d.abs*scale) * (v.abs*scale) = (d.abs*v.abs)*scale**2
	// So after multiplication we should scale result down by division by scale and round it by compare doubled remainder
	// with divisor (scale), and if it grater then add 1 to result (away from zero alg)

	// quick shortcut when both operands have empty hi word
	if d.abs[1] == 0 && v.abs[1] == 0 {
		// Multiply
		result.abs[1], result.abs[0] = bits.Mul64(d.abs[0], v.abs[0])

		// Sacale down
		var r uint64
		result.abs[1], r = bits.Div64(0, result.abs[1], scaleValue)
		result.abs[0], r = bits.Div64(r, result.abs[0], scaleValue)

		// rounding result
		if r*2 >= scaleValue {
			carry := uint64(0)
			result.abs[0], carry = bits.Add64(result.abs[0], 1, carry)
			result.abs[1], carry = bits.Add64(result.abs[1], 0, carry)
			if carry != 0 { // This case inaccessible with correct operands. But for sure keep this here.
				result.form = infDecimal
				return
			}
		}

		return
	}

	// basic multiplication alg with unrolled loop
	var (
		mulBuf                    [4]uint64
		carry, carry2, roundCarry uint64
		res1                      uint64
	)

	// d * v.low
	carry, mulBuf[0] = bits.Mul64(d.abs[0], v.abs[0])
	carry2, res1 = addMul(carry, d.abs[1], v.abs[0]) // carry + (d.abs[1] * v.abs[0])

	// d * v.high
	carry, mulBuf[1] = addMul(res1, d.abs[0], v.abs[1])                 // res1 + (d.abs[0] * v.abs[1])
	mulBuf[3], mulBuf[2] = addAddMul(carry2, d.abs[1], v.abs[1], carry) // carry2 + (d.abs[1] * v.abs[1]) + carry

	// sale down
	var r uint64
	mulBuf[3], r = bits.Div64(r, mulBuf[3], scaleValue)
	mulBuf[2], r = bits.Div64(r, mulBuf[2], scaleValue)
	mulBuf[1], r = bits.Div64(r, mulBuf[1], scaleValue)
	mulBuf[0], r = bits.Div64(r, mulBuf[0], scaleValue)

	// rounding
	if r*2 >= scaleValue {
		mulBuf[0], roundCarry = bits.Add64(mulBuf[0], 1, roundCarry)
		mulBuf[1], roundCarry = bits.Add64(mulBuf[1], 0, roundCarry)
	}

	if roundCarry != 0 || mulBuf[2] != 0 || mulBuf[3] != 0 { // This case inaccessible with correct operands. But for sure keep this here.
		result.form = infDecimal
		return
	}

	result.abs[0], result.abs[1] = mulBuf[0], mulBuf[1]

	return
}

// Div returns result of d-v. In case of some computation errors return value is NaN
func (d Decimal128) Div(v Decimal128) (result Decimal128) {
	switch { // special cases
	case v.IsZero(): // division by 0
		return decimalNan
	case nanOp(d, v): // if one of numbers is NaN
		return decimalNan
	case d.IsInf():
		if v.IsInf() {
			return decimalNan
		}
		return d
	case v.IsInf():
		return
	}

	// d/1 = d
	if v == decimalOne {
		return d
	}

	result.neg = d.neg != v.neg // result will be negative only if signs are different
	defer result.norm()

	// division of values will be looked like:
	//    (d.abs*scale)/(v.abs*scale) = d.abs/v.abs * (0*scale)
	// so we should multiply result by scale and round it.
	// But for more correct result we make this operations this way^
	//    (d.abs*scale) * scale * 2 / (v.abs*scale)*2
	// after this operation we can ger doubled remainder and compare it with v (divisor) for rounding

	// scale is always one word so we can use bits for this operations
	var (
		upscaled [4]uint64 //We need only 3 but add one to use bytes fill functions
	)

	upscaled[1], upscaled[0] = bits.Mul64(d.abs[0], scaleValue)
	upscaled[2], upscaled[1] = addMul(upscaled[1], d.abs[1], scaleValue)

	// mul upscaled d by 2
	const hiBit = (1 << 63)
	upscaled[2] = (upscaled[2] << 1) | ((upscaled[1] & hiBit) >> 63)
	upscaled[1] = (upscaled[1] << 1) | ((upscaled[0] & hiBit) >> 63)
	upscaled[0] = upscaled[0] << 1

	// if doubled divisor will have empty hi word we can use bits division
	if v.abs[1] == 0 && v.abs[0]&hiBit == 0 {
		// mul v by 2
		vs := v.abs[0] << 1

		var r uint64
		upscaled[2], r = bits.Div64(r, upscaled[2], vs)
		if upscaled[2] != 0 { // This case inaccessible with correct operands. But for sure keep this here.
			result.form = infDecimal
			return
		}
		result.abs[1], r = bits.Div64(r, upscaled[1], vs)
		result.abs[0], r = bits.Div64(r, upscaled[0], vs)

		// rounding: here we not multiply reminder by 2 because previously multiply v
		if r >= v.abs[0] {
			carry := uint64(0)
			result.abs[0], carry = bits.Add64(result.abs[0], 1, carry)
			result.abs[1], carry = bits.Add64(result.abs[1], 0, carry)
			if carry != 0 { // This case inaccessible with correct operands. But for sure keep this here.
				result.form = infDecimal
				return
			}
		}
		return
	}

	// general case - use big.Int for division
	x := getInt()
	y := getInt()
	ys := getInt()
	r := getInt()
	defer putInt(x)
	defer putInt(y)
	defer putInt(ys)
	defer putInt(r)

	// copy d * scale * 2 to big.Int (it has double size)
	buf := [byteLen * 2]byte{}
	putUintsToBytesBigEndian(upscaled[2:4], buf[0:byteLen])
	putUintsToBytesBigEndian(upscaled[0:2], buf[byteLen:byteLen*2])
	x.SetBytes(buf[:])

	// copy v to big.Int
	putUintsToBytesBigEndian(v.abs[:], buf[:byteLen])
	y.SetBytes(buf[:byteLen])

	// double v to separate variable
	ys = ys.Lsh(y, 1)
	// make division and round
	x, r = x.QuoRem(x, ys, r)
	if r.CmpAbs(y) >= 0 {
		x = x.Add(x, intOne)
	}
	// check if we overflows decimal
	// really all cases of overflow will go to previous shortcut, because decimalOne has size of one word
	// and all greater value of v cause result less than d
	if x.CmpAbs(maxIntInDecimal) >= 0 { // This case inaccessible with correct operands. But for sure keep this here.
		result.form = infDecimal
		return
	}

	// store result value
	rbuf := [byteLen]byte{}
	_ = x.FillBytes(rbuf[:])
	result.setBytes(rbuf)

	return
}

// IsInt return true if fraction part of d is zero.
func (d Decimal128) IsInt() bool {
	if d.form != finiteDecimal {
		return false
	}
	_, r := bits.Div64(d.abs[1], d.abs[0], scaleValue)
	return r == 0
}

// Modf returns integer and fractional floating-point numbers
// that sum to d. Both values have the same sign as d.
func (d Decimal128) Modf() (i Decimal128, f Decimal128) {
	switch { // special cases
	case d.IsZero():
		return d, d
	case d.IsInf():
		return d, decimalNan
	case d.IsNan():
		return decimalNan, decimalNan
	}
	_, r := bits.Div64(d.abs[1], d.abs[0], scaleValue)
	if r == 0 {
		i = d
		return
	}

	f.neg = d.neg
	f.abs[0] = r
	i = d.Sub(f)
	return
}

// Floor returns the greatest integer value less than or equal to d.
func (d Decimal128) Floor() Decimal128 {
	switch { // special cases
	case d.IsZero():
		return d
	case d.IsInf():
		return d
	case d.IsNan():
		return d
	}

	i, f := d.Modf()
	i.neg = d.neg
	fz := f.IsZero()
	if d.neg && !fz {
		i = i.Sub(decimalOne)
	}
	return i
}

// Floor returns the greatest integer value less than or equal to d.
func (d Decimal128) Ceil() Decimal128 {
	switch { // special cases
	case d.IsZero():
		return d
	case d.IsInf():
		return d
	case d.IsNan():
		return d
	}

	return d.Neg().Floor().Neg()
}

// finiteAbsCmp compare decimal abs (double word) values without looking at sign.
func finiteAbsCmp(d, v [2]uint64) int {
	var dw, vw uint64
	switch {
	case d[1] != v[1]: // if high words are not equal we should compare them
		dw = d[1]
		vw = v[1]
	case d[0] != v[0]: // if low words are not equal - compare them
		dw = d[0]
		vw = v[0]
	default: // we have both words equal
		return 0
	}

	if dw < vw {
		return -1
	}
	return 1
}

// addAddMul computes (hi * 2^64 + lo) = z + (x * y) + carry.
func addAddMul(z, x, y, carry uint64) (hi, lo uint64) {
	hi, lo = bits.Mul64(x, y)
	lo, carry = bits.Add64(lo, carry, 0)
	hi, _ = bits.Add64(hi, 0, carry)
	lo, carry = bits.Add64(lo, z, 0)
	hi, _ = bits.Add64(hi, 0, carry)
	return hi, lo
}

// addMul computes (hi * 2^64 + lo) = z + (x * y)
func addMul(z, x, y uint64) (hi, lo uint64) {
	hi, lo = bits.Mul64(x, y)
	lo, carry := bits.Add64(lo, z, 0)
	hi, _ = bits.Add64(hi, 0, carry)
	return hi, lo
}

func nanOp(x, y Decimal128) bool {
	return x.IsNan() || y.IsNan()
}
