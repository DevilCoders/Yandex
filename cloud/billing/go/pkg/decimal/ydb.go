package decimal

import (
	"errors"
	"fmt"
	"math/bits"

	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
)

// DecimalFromYDB constructs Decimal128 from YDB decimal representation with given precision and scale.
// Largest precision in YDB is 35 and scale is limited by this library implementation to 15.
func DecimalFromYDB(v [16]byte, precision, scale uint32) (result Decimal128, err error) {
	if scale > scaleDecimals {
		err = ErrRange.Wrap(fmt.Errorf("scale %d too large, max allowed scale is %d", precision, scaleDecimals))
		result = decimalNan
		return
	}
	scaleFactor := tenthPowers[scaleDecimals-scale][0]
	maxValue := tenthPowers[precision]
	if precision-scale > precisionDecimals-scaleDecimals { // when integer part can overflow Decimal(38,15)
		maxValue = tenthPowers[precisionDecimals-scaleDecimals+scale]
	}

	defer result.norm()

	// use result.abs as buffer for scale conversions
	putBytesToUintsBigEndian(v[:], result.abs[:])

	result.neg = v[0]&0x80 != 0
	if result.neg { // make complementary int for negative values
		var carry uint64
		result.abs[0], result.abs[1] = ^result.abs[0], ^result.abs[1]
		result.abs[0], carry = bits.Add64(result.abs[0], 1, 0)
		result.abs[1], _ = bits.Add64(result.abs[1], 0, carry)
	}

	switch { // compare with YDB magic numbers and check for overflow
	case finiteAbsCmp(result.abs, infYDB) == 0:
		result.form = infDecimal
		return
	case finiteAbsCmp(result.abs, nanYDB) == 0:
		result = decimalNan
		return
	case finiteAbsCmp(result.abs, errYDB) == 0:
		result = decimalNan
		err = ErrParse.Wrap(errors.New("got ydb error value"))
		return
	case finiteAbsCmp(result.abs, maxValue) >= 0:
		result.form = infDecimal
		err = ErrRange.Wrap(fmt.Errorf("ydb value is too large for conversion from decimal(%d,%d)", precision, scale))
		return
	}

	// scale to 15 digits (input scale checked at function begining)
	var carry uint64
	carry, result.abs[0] = bits.Mul64(result.abs[0], scaleFactor)
	_, result.abs[1] = addMul(carry, result.abs[1], scaleFactor)

	return
}

// YDBProducer is type for quick conversions from Decimal128 to YDB decimal representation with
// given scale and precision.
type YDBProducer struct {
	precision, scale uint32

	maxValue [2]uint64

	ydbType ydb.Type
}

// NewYDBProducer creates YDBProducer with given scale and precision.
// Largest precision in YDB is 35 and scale is limited by this library implementation to 15.
func NewYDBProducer(precision, scale uint32) (*YDBProducer, error) {
	if precision > 35 {
		return nil, ErrRange.Wrap(fmt.Errorf("precision %d too large, max allowed precision is 35", precision))
	}
	if scale > scaleDecimals {
		return nil, ErrRange.Wrap(fmt.Errorf("scale %d too large, max allowed scale is %d", precision, scaleDecimals))
	}

	return &YDBProducer{
		precision: precision,
		scale:     scale,
		maxValue:  tenthPowers[precision],
		ydbType:   ydb.Decimal(precision, scale),
	}, nil
}

// Convert create ydb.Value for d.
func (p *YDBProducer) Convert(d Decimal128) ydb.Value {
	var value [2]uint64
	// convert abs value of d
	switch d.form {
	case infDecimal:
		value = infYDB
	case nanDecimal:
		value = nanYDB
	default: // rescale finite form and check for overflow
		value = scaleFrom15(d.abs, p.scale)
		if finiteAbsCmp(value, p.maxValue) >= 0 {
			value = infYDB
		}
	}

	if d.neg { // make complementary int for negative values
		var carry uint64
		value[0], value[1] = ^value[0], ^value[1]
		value[0], carry = bits.Add64(value[0], 1, 0)
		value[1], _ = bits.Add64(value[1], 0, carry)
	}

	var buf [byteLen]byte
	putUintsToBytesBigEndian(value[:], buf[:])
	return ydb.DecimalValue(p.ydbType, buf)
}

// Check returns d if it not overflows producer's max values and corresponding Inf other case.
func (p *YDBProducer) Check(d Decimal128) (result Decimal128) {
	if d.form != finiteDecimal {
		return d
	}

	defer result.norm()
	scaled := scaleFrom15(d.abs, p.scale)

	if finiteAbsCmp(scaled, p.maxValue) >= 0 {
		d.form = infDecimal
	}
	return d
}

var (
	// YDB magic values
	infYDB = [2]uint64{0x2b878fe800000000, 0x13426172c74d82} // 100000000000000000 * 1000000000000000000
	nanYDB = [2]uint64{0x2b878fe800000001, 0x13426172c74d82} // infYDB + 1
	errYDB = [2]uint64{0x2b878fe800000002, 0x13426172c74d82} // nanYDB + 1
)
