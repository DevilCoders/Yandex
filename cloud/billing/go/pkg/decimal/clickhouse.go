package decimal

import (
	"fmt"
	"math/bits"
)

// DecimalFromClickhouse constructs Decimal128 from ClickHouse decimal representation with given precision and scale.
func DecimalFromClickhouse(v []byte, precision, scale uint32) (result Decimal128, err error) {
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
	putBytesToUintsLittleEndian(v[:], result.abs[:])

	result.neg = v[15]&0x80 != 0
	if result.neg { // make complementary int for negative values
		var carry uint64
		result.abs[0], result.abs[1] = ^result.abs[0], ^result.abs[1]
		result.abs[0], carry = bits.Add64(result.abs[0], 1, 0)
		result.abs[1], _ = bits.Add64(result.abs[1], 0, carry)
	}

	if finiteAbsCmp(result.abs, maxValue) >= 0 {
		result.form = infDecimal
		err = ErrRange.Wrap(fmt.Errorf("clickhouse value is too large for conversion from decimal(%d,%d)", precision, scale))
		return
	}

	// scale to 15 digits (input scale checked at function begining)
	var carry uint64
	carry, result.abs[0] = bits.Mul64(result.abs[0], scaleFactor)
	_, result.abs[1] = addMul(carry, result.abs[1], scaleFactor)

	return
}

// ClickhouseProducer is type for quick conversions from Decimal128 to Clickhouse decimal representation with
// given scale and precision.
type ClickhouseProducer struct {
	precision, scale uint32
	maxValue         [2]uint64
}

// NewClickhouseProducer creates ClickhouseProducer with given scale and precision.
func NewClickhouseProducer(precision, scale uint32) (*ClickhouseProducer, error) {
	if scale > scaleDecimals {
		return nil, ErrRange.Wrap(fmt.Errorf("scale %d too large, max allowed scale is %d", precision, scaleDecimals))
	}

	return &ClickhouseProducer{
		precision: precision,
		scale:     scale,
		maxValue:  tenthPowers[precision],
	}, nil
}

func (c *ClickhouseProducer) Convert(d Decimal128) (buf []byte, err error) {
	var value [2]uint64
	// convert abs value of d

	switch d.form {
	case infDecimal:
		err = ErrInf
		return
	case nanDecimal:
		err = ErrNan
		return
	default: // rescale finite form and check for overflow
		value = scaleFrom15(d.abs, c.scale)
		if finiteAbsCmp(value, c.maxValue) >= 0 {
			err = ErrInf
			return
		}
	}

	if d.neg { // make complementary int for negative values
		var carry uint64
		value[0], value[1] = ^value[0], ^value[1]
		value[0], carry = bits.Add64(value[0], 1, 0)
		value[1], _ = bits.Add64(value[1], 0, carry)
	}

	buf = make([]byte, byteLen)
	putUintsToBytesLittleIndian(value[:], buf[:])
	return
}
