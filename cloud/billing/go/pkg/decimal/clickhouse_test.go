package decimal

import (
	"errors"
	"fmt"
	"testing"

	"github.com/stretchr/testify/suite"
)

type clickhouseSuite struct {
	suite.Suite

	strConstructors
}

func TestClickhouse(t *testing.T) {
	suite.Run(t, new(clickhouseSuite))
}

func (suite *clickhouseSuite) TestCreateProducer() {
	p, err := NewClickhouseProducer(38, 9)
	suite.Require().NoError(err, "actual: %#v", err)
	suite.Require().NotNil(p)

	suite.EqualValues(38, p.precision)
	suite.EqualValues(9, p.scale)
}

func (suite *clickhouseSuite) TestCreateProducerError() {
	_, err := NewClickhouseProducer(22, 16)
	suite.Require().Error(err)
}

func (suite *clickhouseSuite) TestConvertAndCreateFromClickhouse() {
	cases := []struct {
		inp       string
		precision uint32
		scale     uint32
		want      string
	}{
		{"0", 38, 9, "0"},
		{"5.123", 38, 9, "5.123"},
		{"-5.123", 38, 9, "-5.123"},
		{"-12345678901234567890.123456789", 38, 9, "-12345678901234567890.123456789"},
		{"-12345678901234567890.123456789", 38, 9, "-12345678901234567890.123456789"},
		{"1234567890123.456789012", 22, 9, "1234567890123.456789012"},
		{"-1234567890123.456789012", 22, 9, "-1234567890123.456789012"},
	}
	for _, c := range cases {
		suite.Run(fmt.Sprintf("case-%s", c.want), func() {
			converter, err := NewClickhouseProducer(c.precision, c.scale)
			suite.Require().NoError(err)

			bytes, err := converter.Convert(Must(FromString(c.inp)))
			suite.Require().NoError(err)

			d, err := DecimalFromClickhouse(bytes[:], c.precision, c.scale)
			suite.Require().NoError(err, "actual: %#v", err)

			got := d.String()
			suite.Require().Equal(c.want, got)
		})
	}
}

func (suite *clickhouseSuite) TestConvertUnsupportedError() {
	cases := []struct {
		inp       string
		precision uint32
		scale     uint32
		want      error
	}{
		{"NaN", 38, 9, ErrNan},
		{"+Inf", 38, 9, ErrInf},
		{"-Inf", 38, 9, ErrInf},
	}
	for _, c := range cases {
		suite.Run(fmt.Sprintf("case-%s", c.want), func() {
			converter, err := NewClickhouseProducer(c.precision, c.scale)
			suite.Require().NoError(err)

			d, err := converter.Convert(Must(FromString(c.inp)))

			suite.Require().Error(err, "got %f", d)
			suite.True(errors.Is(err, c.want), "actual: %#v", err)
		})
	}
}
