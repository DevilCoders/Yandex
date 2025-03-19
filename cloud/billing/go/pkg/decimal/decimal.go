package decimal

import (
	"fmt"
	"math/big"
	"math/bits"
)

// Must will panics if constructor returns error. Use this helper function if you
// sure that value for construction is correct.
func Must(d Decimal128, err error) Decimal128 {
	if err != nil {
		panic(err)
	}
	return d
}

// Decimal128 is decimal with overall 128 bits to store whole number and 15 decimal digits for scale.
// For simplicity scale is fixed.
//
// Decimal stores number as absolute int value scaled like `x * 10**15`, negative flag and form of number
// for storing special cases like NaN and Inf
//
type Decimal128 struct {
	abs [2]uint64

	form decimalForm
	neg  bool
}

// FromInt64 constructs new Decimal128 value from int64.
func FromInt64(x int64) (Decimal128, error) {
	u := x
	if x < 0 {
		u = -u
	}

	result, _ := FromUInt64(uint64(u))
	if x < 0 {
		result = result.Neg()
	}

	return result, nil
}

// FromUInt64 constructs new Decimal128 value from uint64.
func FromUInt64(x uint64) (Decimal128, error) {
	h, l := bits.Mul64(x, scaleValue)
	result := Decimal128{
		neg: false,
		abs: [2]uint64{l, h},
	}
	result.norm()

	return result, nil
}

// FromBigInt constructs new Decimal128 value from big.Int. Value range will be checked.
func FromBigInt(x *big.Int) (Decimal128, error) {
	i := getInt()
	defer putInt(i)

	i = i.Mul(x, intScale)

	if i.CmpAbs(maxIntInDecimal) >= 0 {
		return decimalNan, ErrRange.Wrap(fmt.Errorf("%d", x))
	}

	result := Decimal128{
		neg: x.Sign() < 0,
	}

	buf := [byteLen]byte{}
	_ = i.FillBytes(buf[:])
	result.setBytes(buf)
	result.norm()

	return result, nil
}

// FromString constructs new Decimal128 value from string representation.
// Only base 10 without exponent values can be parsed. Value can contains underscores which will be ignored.
func FromString(s string) (Decimal128, error) {
	return parse(s)
}

// NewFromFloat64 constructs new Decimal128 value from float64.
// Use carefully because some garbage can be copied from incoming value and value can be rounded.
func NewFromFloat64(x float64) (Decimal128, error) {
	r := getRat()
	defer putRat(r)
	r.SetFloat64(x)

	return FromRat(r)
}

// FromRat constructs new Decimal128 value from big.Rat.
// This constructor is most correct on a par with FromString.
func FromRat(x *big.Rat) (Decimal128, error) {
	r := getRat()
	defer putRat(r)

	r = r.Set(x)

	r = r.Mul(r, ratScale).Abs(r)
	if r.Abs(r).Cmp(maxRatInDecimal) >= 0 {
		return decimalNan, ErrRange.Wrap(fmt.Errorf("%s", x.FloatString(9)))
	}

	result := Decimal128{
		neg: x.Num().Sign() < 0,
	}

	num := getInt()
	denom := getInt()
	shDenom := getInt()
	rem := getInt()
	defer putInt(num)
	defer putInt(denom)
	defer putInt(shDenom)
	defer putInt(rem)

	num.Set(r.Num())
	denom.Set(r.Denom())

	num = num.Lsh(num, 1)
	shDenom = shDenom.Lsh(denom, 1)

	num, rem = num.QuoRem(num, shDenom, rem)
	if rem.CmpAbs(denom) >= 0 {
		num.Add(num, intOne)
	}
	if num.CmpAbs(maxIntInDecimal) >= 0 {
		return decimalNan, ErrRange.Wrap(fmt.Errorf("%s", x.FloatString(9)))
	}

	buf := [byteLen]byte{}
	_ = num.FillBytes(buf[:])
	result.setBytes(buf)
	result.norm()

	return result, nil
}

// IsFinite returns true if Decimal128 contains some finite number.
func (d Decimal128) IsFinite() bool {
	return d.form == finiteDecimal
}

// IsZero returns true if Decimal128 contains zero.
func (d *Decimal128) IsZero() bool {
	return d.form == finiteDecimal && d.abs == [2]uint64{0, 0}
}

// IsNan returns true if Decimal128 contains NaN.
func (d Decimal128) IsNan() bool {
	return d.form == nanDecimal
}

// IsNan returns true if Decimal128 contains Inf or -Inf.
func (d Decimal128) IsInf() bool {
	return d.form == infDecimal
}

type decimalForm uint8

const (
	finiteDecimal = iota
	nanDecimal
	infDecimal
)

func (d *Decimal128) norm() {
	if d.IsZero() {
		d.neg = false
		return
	}

	switch d.form {
	case nanDecimal:
		*d = decimalNan
		return
	case infDecimal:
		d.abs = [2]uint64{}
		return
	}

	if finiteAbsCmp(d.abs, maxDecimal.abs) > 0 {
		d.abs = [2]uint64{}
		d.form = infDecimal
		return
	}
}
