package decimal

import "math/big"

// Float stores representation of d in f. If f is nil then it will be allocated.
func (d Decimal128) Float(f *big.Float) *big.Float {
	r := getRat()
	defer putRat(r)

	r = d.Rat(r)

	if f == nil {
		f = &big.Float{}
	}
	return f.SetPrec(256).SetRat(r)
}

// Rat stores representation of d in r. If r is nil then it will be allocated.
func (d Decimal128) Rat(r *big.Rat) *big.Rat {
	i := getInt()
	defer putInt(i)

	if r == nil {
		r = &big.Rat{}
	}

	r = r.SetInt(d.fillBigInt(i))
	return r.Quo(r, ratScale)
}
