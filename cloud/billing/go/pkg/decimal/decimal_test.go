package decimal

import (
	"errors"
	"fmt"
	"math/big"
	"testing"

	"github.com/stretchr/testify/suite"
)

type decimalCreateSuite struct {
	suite.Suite
}

func TestDecimalCreate(t *testing.T) {
	suite.Run(t, new(decimalCreateSuite))
}

func (suite *decimalCreateSuite) TestMust() {
	suite.Panics(func() { Must(Decimal128{}, fmt.Errorf("error")) })
}

func (suite *decimalCreateSuite) TestIsFinite() {
	cases := []struct {
		inp  Decimal128
		want bool
	}{
		{Must(FromString("0")), true},
		{Must(FromString("1")), true},
		{Must(FromString("Inf")), false},
		{Must(FromString("NaN")), false},
	}
	for _, c := range cases {
		suite.Run(fmt.Sprintf("case-%f", c.inp), func() {
			suite.Equal(c.want, c.inp.IsFinite())
		})
	}
}

func (suite *decimalCreateSuite) TestIsZero() {
	cases := []struct {
		inp  Decimal128
		want bool
	}{
		{Must(FromString("0")), true},
		{Must(FromString("1")), false},
		{Must(FromString("Inf")), false},
		{Must(FromString("NaN")), false},
	}
	for _, c := range cases {
		suite.Run(fmt.Sprintf("case-%f", c.inp), func() {
			suite.Equal(c.want, c.inp.IsZero())
		})
	}
}

func (suite *decimalCreateSuite) TestIsNan() {
	cases := []struct {
		inp  Decimal128
		want bool
	}{
		{Must(FromString("0")), false},
		{Must(FromString("1")), false},
		{Must(FromString("Inf")), false},
		{Must(FromString("NaN")), true},
	}
	for _, c := range cases {
		suite.Run(fmt.Sprintf("case-%f", c.inp), func() {
			suite.Equal(c.want, c.inp.IsNan())
		})
	}
}

func (suite *decimalCreateSuite) TestIsInf() {
	cases := []struct {
		inp  Decimal128
		want bool
	}{
		{Must(FromString("0")), false},
		{Must(FromString("1")), false},
		{Must(FromString("Inf")), true},
		{Must(FromString("NaN")), false},
	}
	for _, c := range cases {
		suite.Run(fmt.Sprintf("case-%f", c.inp), func() {
			suite.Equal(c.want, c.inp.IsInf())
		})
	}
}

func (suite *decimalCreateSuite) TestCreateFromInt() {
	cases := []struct {
		i    int64
		want string
	}{
		{0, "0"},
		{1, "1"},
		{-1, "-1"},
	}
	for _, c := range cases {
		suite.Run(fmt.Sprintf("case-%s", c.want), func() {
			got, err := FromInt64(c.i)
			suite.Require().NoError(err)
			suite.Equal(c.want, got.String())
		})
	}
}

func (suite *decimalCreateSuite) TestCreateFromUInt() {
	cases := []struct {
		i    uint64
		want string
	}{
		{0, "0"},
		{1, "1"},
	}
	for _, c := range cases {
		suite.Run(fmt.Sprintf("case-%s", c.want), func() {
			got, err := FromUInt64(c.i)
			suite.Require().NoError(err)
			suite.Equal(c.want, got.String())
		})
	}
}

func (suite *decimalCreateSuite) TestCreateFromBigInt() {
	maxInt, _ := big.NewInt(0).SetString("99999999999999999999999", 10)
	cases := []struct {
		i    *big.Int
		want string
	}{
		{big.NewInt(0), "0"},
		{big.NewInt(1), "1"},
		{big.NewInt(-1), "-1"},
		{maxInt, "99999999999999999999999"},
	}
	for _, c := range cases {
		suite.Run(fmt.Sprintf("case-%s", c.want), func() {
			got, err := FromBigInt(c.i)
			suite.Require().NoError(err)
			suite.Equal(c.want, got.String())
		})
	}
}

func (suite *decimalCreateSuite) TestCreateFromBigIntOverflow() {
	i, _ := big.NewInt(0).SetString("100000000000000000000000", 10)

	got, err := FromBigInt(i)
	suite.Require().Error(err)
	suite.True(got.IsNan())
	suite.True(errors.Is(err, ErrRange), "actual: %#v", err)
}

func (suite *decimalCreateSuite) TestCreateFromFloat() {
	cases := []struct {
		f    float64
		want string
	}{
		{0, "0"},
		{1.000000001, "1.000000001"},
		{-1.000000001, "-1.000000001"},
	}
	for _, c := range cases {
		suite.Run(fmt.Sprintf("case-%s", c.want), func() {
			got, err := NewFromFloat64(c.f)
			suite.Require().NoError(err)
			suite.Equal(c.want, got.String())
		})
	}
}

func (suite *decimalCreateSuite) TestCreateFromFloatOverflow() {
	got, err := NewFromFloat64(1e25)
	suite.Require().Error(err)
	suite.True(got.IsNan())
	suite.True(errors.Is(err, ErrRange), "actual: %#v", err)
}

func (suite *decimalCreateSuite) TestCreateFromFloatUnderflow() {
	got, err := NewFromFloat64(1e-16)
	suite.Require().NoError(err)
	suite.True(got.IsZero())
}

func (suite *decimalCreateSuite) TestCreateFromRat() {
	maxRat, _ := big.NewRat(0, 1).SetString("99999999999999999999999.999999999999999")
	cases := []struct {
		r    *big.Rat
		want string
	}{
		{(&big.Rat{}).SetFloat64(0), "0"},
		{(&big.Rat{}).SetFloat64(1.000000001), "1.000000001"},
		{(&big.Rat{}).SetFloat64(-1.000000001), "-1.000000001"},
		{(&big.Rat{}).SetFloat64(0.0000000000000005), "0.000000000000001"},
		{(&big.Rat{}).SetFloat64(0.0000000000000001), "0"},
		{maxRat, "99999999999999999999999.999999999999999"},
	}
	for _, c := range cases {
		suite.Run(fmt.Sprintf("case-%s", c.want), func() {
			got, err := FromRat(c.r)
			suite.Require().NoError(err)
			suite.Equal(c.want, got.String())
		})
	}
}

func (suite *decimalCreateSuite) TestCreateFromRatOverflow() {
	r, _ := big.NewRat(0, 1).SetString("100000000000000000000000")

	got, err := FromRat(r)
	suite.Require().Error(err)
	suite.True(got.IsNan())
	suite.True(errors.Is(err, ErrRange), "actual: %#v", err)
}

func (suite *decimalCreateSuite) TestCreateFromRatRoundOverflow() {
	r, _ := big.NewRat(0, 1).SetString("99999999999999999999999.9999999999999999")

	got, err := FromRat(r)
	suite.Require().Error(err)
	suite.True(got.IsNan())
	suite.True(errors.Is(err, ErrRange), "actual: %#v", err)
}

func (suite *decimalCreateSuite) TestCreateFromString() {
	x := Must(FromString("1.000000001"))

	str := x.String()
	suite.Require().Equal("1.000000001", str)
}

func (suite *decimalCreateSuite) TestIntVSFloat() {
	cases := []struct {
		i int64
		f float64
	}{
		{1844684407, 1844684407},
	}
	for i, c := range cases {
		suite.Run(fmt.Sprintf("case-%d", i), func() {
			di := Must(FromInt64(c.i))
			df := Must(NewFromFloat64(c.f))
			suite.Zero(di.Cmp(df), fmt.Sprintf("%f %f", di, df))
		})
	}
}
