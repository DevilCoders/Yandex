package decimal

import "math/big"

const (
	precisionDecimals = 38
	scaleDecimals     = 15
	byteLen           = 8 * 2

	// 2**53 - 1 is max safe mant value for float64 and contains 16 decimal places
	// so if we have less than 16 digits then set unquoted value in encoders, else quote value
	unquotedDecimals = 15
)

var (
	// Scaling constants

	// all magic will not work if scale value will be more than uint64 can store
	scaleValue = func() uint64 {
		i := (&big.Int{}).Exp(big.NewInt(10), big.NewInt(scaleDecimals), nil)
		if !i.IsUint64() {
			panic("scale too big for uint64 value")
		}
		return i.Uint64()
	}()

	ratScale = (&big.Rat{}).SetUint64(scaleValue)
	intScale = (&big.Int{}).SetUint64(scaleValue)

	// Overflow control constants
	maxIntInDecimal = big.NewInt(0).Exp(big.NewInt(10), big.NewInt(precisionDecimals), nil)
	maxRatInDecimal = (&big.Rat{}).SetInt(maxIntInDecimal)
	maxDecimal      = func() (result Decimal128) {
		num := big.NewInt(0).Sub(maxIntInDecimal, big.NewInt(1))
		buf := [byteLen]byte{}
		_ = num.FillBytes(buf[:])
		result.setBytes(buf)
		return
	}()

	// Rounding constants
	intOne = big.NewInt(1)

	decimalOne = Decimal128{abs: [2]uint64{scaleValue, 0}}
	decimalNan = Decimal128{form: nanDecimal}
	decimalInf = Decimal128{form: infDecimal}

	// Strings constants
	zerosBuf = func() (r [precisionDecimals]byte) {
		for i := range r {
			r[i] = '0'
		}
		return
	}()
)
