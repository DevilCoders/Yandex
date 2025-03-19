package decimal

import (
	"bytes"
	"errors"
	"fmt"
	"testing"

	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/decimal"
	"github.com/stretchr/testify/suite"
)

type ydbSuite struct {
	suite.Suite

	strConstructors
}

func TestYDB(t *testing.T) {
	suite.Run(t, new(ydbSuite))
}

func (suite *ydbSuite) TestCreateFromYDB() {
	cases := []struct {
		inp       string
		precision uint32
		scale     uint32
		want      string
	}{
		{"0", 35, 10, "0"},
		{"12345678901234567890.123456789012345", 35, 15, "12345678901234567890.123456789012345"},
		{"-12345678901234567890.123456789012345", 35, 15, "-12345678901234567890.123456789012345"},
		{"1234567890123.456789012", 22, 9, "1234567890123.456789012"},
		{"-1234567890123.456789012", 22, 9, "-1234567890123.456789012"},
		{"Inf", 22, 9, "+Inf"},
		{"-Inf", 22, 9, "-Inf"},
		{"NaN", 22, 9, "NaN"},
	}
	for _, c := range cases {
		suite.Run(fmt.Sprintf("case-%s", c.want), func() {
			i, err := decimal.Parse(c.inp, c.precision, c.scale)
			suite.Require().NoError(err, "actual: %#v", err)
			suite.Require().NotNil(i)

			bytes := decimal.Int128(i, c.precision, c.scale)

			d, err := DecimalFromYDB(bytes, c.precision, c.scale)
			suite.Require().NoError(err, "actual: %#v", err)

			got := d.String()
			suite.Equal(c.want, got)
		})
	}
}

func (suite *ydbSuite) TestCreateFromYDBRangeError() {
	cases := []struct {
		inp       string
		precision uint32
		scale     uint32
		want      error
	}{
		{"0", 35, 20, ErrRange},
		{"1234567890123456789012345.6789012345", 35, 10, ErrRange},
	}
	for _, c := range cases {
		suite.Run(fmt.Sprintf("case-%s", c.want), func() {
			i, err := decimal.Parse(c.inp, c.precision, c.scale)
			suite.Require().NoError(err, "actual: %#v", err)
			suite.Require().NotNil(i)

			bytes := decimal.Int128(i, c.precision, c.scale)

			d, err := DecimalFromYDB(bytes, c.precision, c.scale)
			suite.Require().Error(err, "got %f", d)
			suite.True(errors.Is(err, c.want), "actual: %#v", err)
		})
	}
}

func (suite *ydbSuite) TestCreateFromYDBError() {
	bytes := decimal.Int128(decimal.Err(), 22, 9)

	d, err := DecimalFromYDB(bytes, 22, 9)
	suite.Require().Error(err, "got %f", d)
	suite.True(errors.Is(err, ErrParse), "actual: %#v", err)
}

func (suite *ydbSuite) TestCreateProducer() {
	p, err := NewYDBProducer(35, 15)
	suite.Require().NoError(err, "actual: %#v", err)
	suite.Require().NotNil(p)

	suite.EqualValues(35, p.precision)
	suite.EqualValues(15, p.scale)

	buf := bytes.NewBuffer(nil)
	ydb.WriteTypeStringTo(buf, p.ydbType)
	suite.Equal("Decimal(35,15)", buf.String())
}

func (suite *ydbSuite) TestCreateProducerError() {
	_, err := NewYDBProducer(36, 15)
	suite.Require().Error(err)

	_, err = NewYDBProducer(22, 16)
	suite.Require().Error(err)
}

func (suite *ydbSuite) TestCheck() {
	cases := []struct {
		inp       Decimal128
		precision uint32
		scale     uint32
		want      Decimal128
	}{
		{suite.decimal("1"), 22, 9, suite.decimal("1")},
		{suite.decimal("1234567890123.456789012"), 22, 9, suite.decimal("1234567890123.456789012")},
		{suite.decimal("991234567890123.456789012"), 22, 9, suite.decimal("Inf")},
		{suite.decimal("-991234567890123.456789012"), 22, 9, suite.decimal("-Inf")},
		{suite.decimal("Inf"), 22, 9, suite.decimal("Inf")},
		{suite.decimal("NaN"), 22, 9, suite.decimal("NaN")},
	}
	for _, c := range cases {
		suite.Run(fmt.Sprintf("case-%s", c.want), func() {
			p, err := NewYDBProducer(c.precision, c.scale)
			suite.Require().NoError(err)

			got := p.Check(c.inp)
			suite.Zero(c.want.Cmp(got), got.String())
		})
	}
}

func (suite *ydbSuite) TestConvert() {
	p, err := NewYDBProducer(22, 9)
	suite.Require().NoError(err)

	makeDecimal := func(s string) ydb.Value {
		v, err := decimal.Parse(s, 22, 9)
		suite.Require().NoError(err)
		return ydb.DecimalValue(ydb.Decimal(22, 9), decimal.Int128(v, 22, 9))
	}

	cases := []struct {
		inp  Decimal128
		want ydb.Value
	}{
		{suite.decimal("1"), makeDecimal("1")},
		{suite.decimal("1234567890123.456789012"), makeDecimal("1234567890123.456789012")},
		{suite.decimal("1.000000000_5"), makeDecimal("1.000000001")},
		{suite.decimal("1.000000000_6"), makeDecimal("1.000000001")},
		{suite.decimal("1.000000000_4"), makeDecimal("1")},
		{suite.decimal("991234567890123.456789012"), makeDecimal("Inf")},
		{suite.decimal("-991234567890123.456789012"), makeDecimal("-Inf")},
		{suite.decimal("Inf"), makeDecimal("Inf")},
		{suite.decimal("NaN"), makeDecimal("NaN")},
	}
	for _, c := range cases {
		suite.Run(fmt.Sprintf("case-%s", c.inp), func() {
			got := p.Convert(c.inp)
			gotString := got.(interface{ String() string }).String()
			wantString := c.want.(interface{ String() string }).String()

			suite.Equal(wantString, gotString)
		})
	}
}
