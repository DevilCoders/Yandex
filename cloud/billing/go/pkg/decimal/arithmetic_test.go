package decimal

import (
	"fmt"
	"testing"

	"github.com/stretchr/testify/suite"
)

type decimalOpsSuite struct {
	suite.Suite
}

func TestDecimalOps(t *testing.T) {
	suite.Run(t, new(decimalOpsSuite))
}

func (suite *decimalOpsSuite) TestSigns() {
	cases := []struct {
		x           Decimal128
		want        Decimal128
		wantSign    int
		wantSignBit bool
	}{
		{Must(FromInt64(0)), Must(FromInt64(0)), 0, false},
		{Must(FromInt64(1)), Must(FromInt64(1)), 1, false},
		{Must(FromInt64(-1)), Must(FromInt64(1)), -1, true},
		{Must(FromString("NaN")), Must(FromString("NaN")), 0, false},
		{Must(FromString("Inf")), Must(FromString("Inf")), 1, false},
		{Must(FromString("-Inf")), Must(FromString("Inf")), -1, true},
	}

	for i, c := range cases {
		suite.Run(fmt.Sprintf("case-%d", i), func() {
			got := c.x.Abs()
			gotSign := c.x.Sign()
			gotBit := c.x.Signbit()

			suite.Zero(c.want.Cmp(got), got.String)
			suite.Equal(c.wantSign, gotSign)
			suite.Equal(c.wantSignBit, gotBit)
		})
	}
}

func (suite *decimalOpsSuite) TestCmp() {
	cases := []struct {
		x, y Decimal128
		want int
	}{
		{Must(FromInt64(0)), Must(FromInt64(0)), 0},
		{Must(FromInt64(1)), Must(FromInt64(1)), 0},
		{
			Must(FromString("99999999999999999999999")),
			Must(FromString("99999999999999999999999")),
			0,
		},
		{Must(FromInt64(-1)), Must(FromInt64(1)), -1},
		{Must(FromInt64(1)), Must(FromInt64(-1)), 1},
		{Must(FromInt64(-2)), Must(FromInt64(-1)), -1},
		{Must(FromString("NaN")), Must(FromString("NaN")), 0},
		{Must(FromString("NaN")), Must(FromInt64(1)), 0},
		{Must(FromInt64(1)), Must(FromString("NaN")), 0},
		{Must(FromString("Inf")), Must(FromString("Inf")), 0},
		{Must(FromString("-Inf")), Must(FromString("Inf")), -1},
		{Must(FromString("Inf")), Must(FromString("-Inf")), 1},
		{Must(FromString("-Inf")), Must(FromString("-Inf")), 0},
		{Must(FromString("Inf")), Must(FromInt64(1)), 1},
		{Must(FromString("-Inf")), Must(FromInt64(-1)), -1},
		{Must(FromInt64(1)), Must(FromString("Inf")), -1},
		{Must(FromInt64(1)), Must(FromString("-Inf")), 1},
		{Must(FromInt64(-1)), Must(FromString("Inf")), -1},
		{Must(FromInt64(-1)), Must(FromString("-Inf")), 1},
	}

	for i, c := range cases {
		suite.Run(fmt.Sprintf("case-%d", i), func() {
			got := c.x.Cmp(c.y)
			suite.Equal(c.want, got)
		})
	}
}

func (suite *decimalOpsSuite) TestAdd() {
	cases := []struct {
		x, y Decimal128
		want Decimal128
	}{
		{Must(FromInt64(1)), Must(FromInt64(10)), Must(FromInt64(11))},
		{Must(FromInt64(1)), Must(FromInt64(-10)), Must(FromInt64(-9))},
		{Must(FromInt64(-1)), Must(FromInt64(10)), Must(FromInt64(9))},
		{Must(FromInt64(-1)), Must(FromInt64(-10)), Must(FromInt64(-11))},
		{Must(FromInt64(0)), Must(FromInt64(1)), Must(FromInt64(1))},
		{Must(FromInt64(1)), Must(FromInt64(0)), Must(FromInt64(1))},
		{
			Must(NewFromFloat64(0.0000000003)),
			Must(NewFromFloat64(0.0000000002)),
			Must(NewFromFloat64(0.0000000005)),
		},
		{
			Must(NewFromFloat64(3000000000000000000000)),
			Must(NewFromFloat64(2000000000000000000000)),
			Must(NewFromFloat64(5000000000000000000000)),
		},
		{
			Must(NewFromFloat64(10000000000000000000000)),
			Must(NewFromFloat64(0.000000000000001)),
			Must(FromString("10000000000000000000000.000000000000001")),
		},
		{
			Must(FromString("99999999999999999999999.999999999999999")),
			Must(FromString("0.000000000000001")),
			Must(FromString("Inf")),
		},
		{
			Must(FromString("99999999999999999999999.999999999999999")),
			Must(FromString("99999999999999999999999.999999999999999")),
			Must(FromString("Inf")),
		},
		{Must(FromString("NaN")), Must(FromInt64(0)), Must(FromString("NaN"))},
		{Must(FromInt64(1)), Must(FromString("NaN")), Must(FromString("NaN"))},
		{Must(FromString("Inf")), Must(FromInt64(0)), Must(FromString("Inf"))},
		{Must(FromInt64(1)), Must(FromString("Inf")), Must(FromString("Inf"))},
	}

	for i, c := range cases {
		suite.Run(fmt.Sprintf("case-%d", i), func() {
			got := c.x.Add(c.y)
			suite.Zero(c.want.Cmp(got), got.String())
		})
	}
}

func (suite *decimalOpsSuite) TestSub() {
	cases := []struct {
		x, y Decimal128
		want Decimal128
	}{
		{Must(FromInt64(0)), Must(FromInt64(0)), Must(FromInt64(0))},
		{Must(FromInt64(1)), Must(FromInt64(1)), Must(FromInt64(0))},
		{Must(FromInt64(1)), Must(FromInt64(10)), Must(FromInt64(-9))},
		{Must(FromInt64(1)), Must(FromInt64(-10)), Must(FromInt64(11))},
		{Must(FromInt64(-1)), Must(FromInt64(10)), Must(FromInt64(-11))},
		{Must(FromInt64(-1)), Must(FromInt64(-10)), Must(FromInt64(9))},
		{Must(FromInt64(10)), Must(FromInt64(1)), Must(FromInt64(9))},
		{Must(FromInt64(10)), Must(FromInt64(-1)), Must(FromInt64(11))},
		{Must(FromInt64(-10)), Must(FromInt64(1)), Must(FromInt64(-11))},
		{Must(FromInt64(-10)), Must(FromInt64(-1)), Must(FromInt64(-9))},
		{Must(FromInt64(0)), Must(FromInt64(1)), Must(FromInt64(-1))},
		{Must(FromInt64(1)), Must(FromInt64(0)), Must(FromInt64(1))},
		{
			Must(NewFromFloat64(0.0000000003)),
			Must(NewFromFloat64(0.0000000002)),
			Must(NewFromFloat64(0.0000000001)),
		},
		{
			Must(NewFromFloat64(0.0000000002)),
			Must(NewFromFloat64(0.0000000003)),
			Must(NewFromFloat64(-0.0000000001)),
		},
		{
			Must(NewFromFloat64(3000000000000000000000)),
			Must(NewFromFloat64(2000000000000000000000)),
			Must(NewFromFloat64(1000000000000000000000)),
		},
		{
			Must(NewFromFloat64(10000000000000000000000)),
			Must(NewFromFloat64(0.000000000000001)),
			Must(FromString("9999999999999999999999.999999999999999")),
		},
		{
			Must(FromString("-99999999999999999999999.999999999999999")),
			Must(FromString("0.000000000000001")),
			Must(FromString("-Inf")),
		},
		{Must(FromString("NaN")), Must(FromInt64(0)), Must(FromString("NaN"))},
		{Must(FromInt64(1)), Must(FromString("NaN")), Must(FromString("NaN"))},
		{Must(FromString("Inf")), Must(FromInt64(0)), Must(FromString("Inf"))},
		{Must(FromInt64(1)), Must(FromString("Inf")), Must(FromString("-Inf"))},
	}

	for i, c := range cases {
		suite.Run(fmt.Sprintf("case-%d", i), func() {
			z := c.x.Sub(c.y)
			suite.Zero(c.want.Cmp(z), z.String())
		})
	}
}

func (suite *decimalOpsSuite) TestMul() {
	cases := []struct {
		x, y Decimal128
		want Decimal128
	}{
		{Must(FromInt64(0)), Must(FromInt64(1)), Must(FromInt64(0))},
		{Must(FromInt64(1)), Must(FromInt64(0)), Must(FromInt64(0))},
		{Must(FromInt64(5)), Must(FromInt64(1)), Must(FromInt64(5))},
		{Must(FromInt64(1)), Must(FromInt64(5)), Must(FromInt64(5))},
		{Must(FromInt64(2)), Must(FromInt64(3)), Must(FromInt64(6))},
		{Must(FromInt64(-2)), Must(FromInt64(3)), Must(FromInt64(-6))},
		{
			Must(FromInt64(9999999999999)),
			Must(FromInt64(3333333333)),
			Must(FromString("33333333329996666666667")),
		},
		{
			Must(NewFromFloat64(0.0003)),
			Must(NewFromFloat64(0.00002)),
			Must(NewFromFloat64(0.000000006)),
		},
		{
			Must(NewFromFloat64(10000000000000000000000)),
			Must(NewFromFloat64(0.000000000000001)),
			Must(FromString("10000000")),
		},
		{
			Must(NewFromFloat64(0.00000000001)),
			Must(NewFromFloat64(0.00000000001)),
			Must(FromInt64(0)),
		},
		{
			Must(NewFromFloat64(10000000000000000000000)),
			Must(NewFromFloat64(10)),
			Must(FromString("Inf")),
		},
		{
			Must(NewFromFloat64(10000000000000000000000)),
			Must(NewFromFloat64(-10)),
			Must(FromString("-Inf")),
		},
		{Must(FromString("NaN")), Must(FromInt64(0)), Must(FromString("NaN"))},
		{Must(FromInt64(1)), Must(FromString("NaN")), Must(FromString("NaN"))},
		{Must(FromString("Inf")), Must(FromInt64(0)), Must(FromString("Inf"))},
		{Must(FromInt64(1)), Must(FromString("Inf")), Must(FromString("Inf"))},
		{
			Must(FromString("10000000000000000000000.000000000000001")),
			Must(FromString("0.5")),
			Must(FromString("5000000000000000000000.000000000000001")),
		},
	}

	for i, c := range cases {
		suite.Run(fmt.Sprintf("case-%d", i), func() {
			z := c.x.Mul(c.y)
			suite.Zero(c.want.Cmp(z), z.String())
		})
	}
}

func (suite *decimalOpsSuite) TestDiv() {
	cases := []struct {
		x, y Decimal128
		want Decimal128
	}{
		{Must(FromInt64(0)), Must(FromInt64(1)), Must(FromInt64(0))},
		{Must(FromInt64(6)), Must(FromInt64(2)), Must(FromInt64(3))},
		{Must(FromInt64(1)), Must(FromInt64(4)), Must(NewFromFloat64(0.25))},
		{Must(FromInt64(-9)), Must(FromInt64(3)), Must(FromInt64(-3))},
		{Must(FromInt64(9)), Must(FromInt64(-3)), Must(FromInt64(-3))},
		{Must(FromInt64(30)), Must(FromInt64(3600)), Must(NewFromFloat64(0.008333333333333))},
		{Must(FromInt64(45)), Must(FromInt64(3600)), Must(NewFromFloat64(0.0125))},
		{Must(FromInt64(1)), Must(FromInt64(3)), Must(NewFromFloat64(0.333333333333333))},
		{Must(FromInt64(2)), Must(FromInt64(3)), Must(NewFromFloat64(0.666666666666667))},
		{Must(FromInt64(1)), Must(FromInt64(2_000_000_000_000_000)), Must(NewFromFloat64(0.000000000000001))},
		{Decimal128{abs: [2]uint64{0, 60}}, Decimal128{abs: [2]uint64{0, 2}}, Must(FromInt64(30))},
		{
			Must(NewFromFloat64(1)),
			Must(NewFromFloat64(0)),
			Must(FromString("NaN")),
		},
		{
			Must(NewFromFloat64(0.0003)),
			Must(NewFromFloat64(0.00002)),
			Must(NewFromFloat64(15)),
		},
		{
			Must(NewFromFloat64(10000000000000000000000)),
			Must(NewFromFloat64(0.1)),
			Must(FromString("Inf")),
		},
		{
			Must(NewFromFloat64(10000000000000000000000)),
			Must(NewFromFloat64(-0.1)),
			Must(FromString("-Inf")),
		},
		{Must(FromString("NaN")), Must(FromInt64(1)), Must(FromString("NaN"))},
		{Must(FromInt64(1)), Must(FromString("NaN")), Must(FromString("NaN"))},
		{Must(FromString("Inf")), Must(FromInt64(1)), Must(FromString("Inf"))},
		{Must(FromInt64(1)), Must(FromString("Inf")), Must(FromInt64(0))},
		{Must(FromString("Inf")), Must(FromString("Inf")), Must(FromString("NaN"))},
	}

	for i, c := range cases {
		suite.Run(fmt.Sprintf("case-%d", i), func() {
			z := c.x.Div(c.y)
			suite.Zero(c.want.Cmp(z), z.String())
		})
	}
}

func (suite *decimalOpsSuite) TestIsInt() {
	cases := []struct {
		x    Decimal128
		want bool
	}{
		{Must(FromString("0")), true},
		{Must(FromString("1")), true},
		{Must(FromString("-1")), true},
		{Must(FromString("Inf")), false},
		{Must(FromString("-Inf")), false},
		{Must(FromString("Nan")), false},
		{Must(FromString("0.5")), false},
		{Must(FromString("999.5")), false},
		{Must(FromString("-0.5")), false},
		{Must(FromString("-999.5")), false},
	}

	for _, c := range cases {
		suite.Run(fmt.Sprintf("case-%f", c.x), func() {
			z := c.x.IsInt()
			suite.Equal(c.want, z)
		})
	}
}

func (suite *decimalOpsSuite) TestModf() {
	cases := []struct {
		x        Decimal128
		wantInt  Decimal128
		wantFrac Decimal128
	}{
		{Must(FromString("0")), Must(FromString("0")), Must(FromString("0"))},
		{Must(FromString("1")), Must(FromString("1")), Must(FromString("0"))},
		{Must(FromString("-1")), Must(FromString("-1")), Must(FromString("0"))},
		{Must(FromString("Inf")), Must(FromString("Inf")), Must(FromString("NaN"))},
		{Must(FromString("-Inf")), Must(FromString("-Inf")), Must(FromString("NaN"))},
		{Must(FromString("NaN")), Must(FromString("NaN")), Must(FromString("NaN"))},
		{Must(FromString("0.5")), Must(FromString("0")), Must(FromString("0.5"))},
		{Must(FromString("999.5")), Must(FromString("999")), Must(FromString("0.5"))},
		{Must(FromString("-0.5")), Must(FromString("0")), Must(FromString("-0.5"))},
		{Must(FromString("-999.5")), Must(FromString("-999")), Must(FromString("-0.5"))},
	}

	for _, c := range cases {
		suite.Run(fmt.Sprintf("case-%d", c.x), func() {
			i, f := c.x.Modf()
			suite.Zero(i.Cmp(c.wantInt), i.String())
			suite.Zero(f.Cmp(c.wantFrac), f.String())
		})
	}
}

func (suite *decimalOpsSuite) TestFloor() {
	cases := []struct {
		x    Decimal128
		want Decimal128
	}{
		{Must(FromString("0")), Must(FromString("0"))},
		{Must(FromString("1")), Must(FromString("1"))},
		{Must(FromString("-1")), Must(FromString("-1"))},
		{Must(FromString("Inf")), Must(FromString("Inf"))},
		{Must(FromString("-Inf")), Must(FromString("-Inf"))},
		{Must(FromString("NaN")), Must(FromString("NaN"))},
		{Must(FromString("0.5")), Must(FromString("0"))},
		{Must(FromString("999.5")), Must(FromString("999"))},
		{Must(FromString("-0.5")), Must(FromString("-1"))},
		{Must(FromString("-999.5")), Must(FromString("-1000"))},
	}

	for _, c := range cases {
		suite.Run(fmt.Sprintf("case-%f", c.x), func() {
			f := c.x.Floor()
			suite.Zero(f.Cmp(c.want), f.String())
		})
	}
}

func (suite *decimalOpsSuite) TestCeil() {
	cases := []struct {
		x    Decimal128
		want Decimal128
	}{
		{Must(FromString("0")), Must(FromString("0"))},
		{Must(FromString("1")), Must(FromString("1"))},
		{Must(FromString("-1")), Must(FromString("-1"))},
		{Must(FromString("Inf")), Must(FromString("Inf"))},
		{Must(FromString("-Inf")), Must(FromString("-Inf"))},
		{Must(FromString("NaN")), Must(FromString("NaN"))},
		{Must(FromString("0.5")), Must(FromString("1"))},
		{Must(FromString("999.5")), Must(FromString("1000"))},
		{Must(FromString("-0.5")), Must(FromString("0"))},
		{Must(FromString("-999.5")), Must(FromString("-999"))},
	}

	for _, c := range cases {
		suite.Run(fmt.Sprintf("case-%f", c.x), func() {
			f := c.x.Ceil()
			suite.Zero(f.Cmp(c.want), f.String())
		})
	}
}

func (suite *decimalOpsSuite) TestUnderflowDivRound() {
	one := Must(FromInt64(1))
	three := Must(FromInt64(3))
	half := Must(NewFromFloat64(0.5))

	result := one.Add(half).Div(three)

	str := result.String()
	suite.Equal("0.5", str)
}

func (suite *decimalOpsSuite) TestUnderflowMulRound() {
	one := Must(FromInt64(1))
	three := Must(FromInt64(3))
	half := Must(NewFromFloat64(0.5))

	oneThird := one.Div(three)
	result := one.Add(half).Mul(oneThird)

	str := result.String()
	suite.Equal("0.5", str)
}
