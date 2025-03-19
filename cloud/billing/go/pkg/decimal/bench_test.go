package decimal

import (
	"fmt"
	"math/big"
	"math/rand"
	"testing"
)

var rnd = rand.New(rand.NewSource(0))

func randDecimal() Decimal128 {
	return Decimal128{
		abs: [2]uint64{
			uint64(rnd.Int63()<<1 | rnd.Int63n(2)),
			0,
		},
		neg: rnd.Int63n(2) == 1,
	}
}

func randBigDecimal() Decimal128 {
	if rand.Int31n(2) == 0 {
		return Decimal128{
			abs: [2]uint64{
				uint64(rnd.Int63()<<1 | rnd.Int63n(2)),
				0,
			},
			neg: rnd.Int63n(2) == 1,
		}
	}
	return Decimal128{
		abs: [2]uint64{
			uint64(rnd.Int63()<<1 | rnd.Int63n(2)),
			uint64(rnd.Int63()<<1 | rnd.Int63n(2)),
		},
		neg: rnd.Int63n(2) == 1,
	}
}

func randBigDecimalNotZero() Decimal128 {
	for {
		d := randBigDecimal()
		if !d.IsZero() {
			return d
		}
	}
}

func BenchmarkAddSub(b *testing.B) {
	da := randBigDecimal()
	db := randDecimal()
	dc := randBigDecimal()

	for i := 0; i < b.N; i++ {
		da.Add(db).Sub(dc)
	}
}

func BenchmarkMul(b *testing.B) {
	da := randDecimal()
	db := randDecimal()

	for i := 0; i < b.N; i++ {
		da.Mul(db)
	}
}

func BenchmarkDiv(b *testing.B) {
	da := randBigDecimal()
	db := randBigDecimal()
	if db.Cmp(da) > 0 {
		da, db = db, da
	}

	for i := 0; i < b.N; i++ {
		da.Div(db)
	}
}

func BenchmarkArithmetic(b *testing.B) {
	da := randDecimal()
	db := randDecimal()
	dc := randDecimal()
	dd := randBigDecimal()
	de := randBigDecimalNotZero()

	for i := 0; i < b.N; i++ {
		da.Mul(db).Add(dc).Sub(dd).Div(de)
	}
}

func BenchmarkDiscount(b *testing.B) {
	discountLv1 := Must(NewFromFloat64(0.8))
	discountLv2 := Must(NewFromFloat64(0.5))
	discountThreshold := Must(FromInt64(1000))
	price := Must(FromInt64(15))

	want := Must(FromInt64(12875))

	for i := 0; i < b.N; i++ {
		var total Decimal128

		for j := 0; j < 100000; j++ {
			quant := Must(FromInt64(60))
			coeff := Must(FromInt64(3600))

			disc := discountLv2
			if total.Cmp(discountThreshold) < 0 {
				disc = discountLv1
			}
			discountedCost := quant.Mul(price).Mul(disc).Div(coeff)
			total = total.Add(discountedCost)
		}
		if total.Cmp(want) != 0 {
			panic(fmt.Sprintf("incorrect result: %37.15f", total.Float(nil)))
		}
	}
}

func BenchmarkDiscountLarge(b *testing.B) {
	discountLv1 := Must(NewFromFloat64(0.8))
	discountLv2 := Must(NewFromFloat64(0.5))
	discountThreshold := Must(FromInt64(1000))
	price := Must(FromInt64(15_000_000))

	for i := 0; i < b.N; i++ {
		var total Decimal128

		for j := 0; j < 100000; j++ {
			quant := Must(FromInt64(60))
			coeff := Must(FromInt64(3600))

			disc := discountLv2
			if total.Cmp(discountThreshold) < 0 {
				disc = discountLv1
			}
			discountedCost := quant.Mul(price).Mul(disc).Div(coeff)
			total = total.Add(discountedCost)
		}
	}
}

func BenchmarkNewFromInt(b *testing.B) {
	val := int64(123456789)

	for i := 0; i < b.N; i++ {
		_, _ = FromInt64(val)
	}
}

func BenchmarkNewFromBigInt(b *testing.B) {
	val := (&big.Int{}).SetInt64(-123456789)

	for i := 0; i < b.N; i++ {
		_, _ = FromBigInt(val)
	}
}

func BenchmarkNewFromFloat(b *testing.B) {
	val := float64(-123456789.123456789)

	for i := 0; i < b.N; i++ {
		_, _ = NewFromFloat64(val)
	}
}

func BenchmarkNewFromRat(b *testing.B) {
	val := (&big.Rat{}).SetFloat64(-123456789.123456789)

	for i := 0; i < b.N; i++ {
		_, _ = FromRat(val)
	}
}

func BenchmarkNewFromString(b *testing.B) {
	val := "-123456789.123456789"

	for i := 0; i < b.N; i++ {
		_, _ = FromString(val)
	}
}
