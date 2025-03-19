package decimal

import (
	"errors"
	"fmt"
	"math/big"
	"testing"

	"github.com/stretchr/testify/suite"
)

type parseSuite struct {
	suite.Suite
}

func TestParse(t *testing.T) {
	suite.Run(t, new(parseSuite))
}

func (suite *parseSuite) TestParse() {
	parseBig := func(s string) *big.Rat {
		v, _ := (&big.Rat{}).SetString(s)
		return v
	}

	cases := []struct {
		inp  string
		want Decimal128
	}{
		{"0", Must(FromInt64(0))},
		{"-0", Must(FromInt64(0))},
		{"1.000000000000001", Must(NewFromFloat64(1.000000000000001))},
		{"1.0000000000000001", Must(FromInt64(1))},
		{"99999999999999999999999", Must(FromRat(parseBig("99999999999999999999999")))},
		{"99_999_999_999_999_999_999_999.", Must(FromRat(parseBig("99999999999999999999999")))},
		{
			"99_999_999_999_999_999_999_999.888_888_888_888_888_123123",
			Must(FromRat(parseBig("99999999999999999999999.888888888888888"))),
		},
		{"NaN", decimalNan},
		{"Nan", decimalNan},
		{"nan", decimalNan},
		{"Inf", decimalInf},
		{"inf", decimalInf},
		{"+Inf", decimalInf},
		{"+inf", decimalInf},
		{"-Inf", decimalInf.Neg()},
		{"-inf", decimalInf.Neg()},
	}
	for _, c := range cases {
		suite.Run(fmt.Sprintf("case-%s", c.inp), func() {
			got, err := parse(c.inp)
			suite.Require().NoError(err)
			suite.Equal(c.want, got)
		})
	}
}

func (suite *parseSuite) TestParseErrors() {

	cases := []struct {
		inp string
		err error
	}{
		{"x", ErrSyntax},
		{"-x", ErrSyntax},
		{"--1", ErrSyntax},
		{"-+1", ErrSyntax},
		{"1-", ErrSyntax},
		{"1+", ErrSyntax},
		{"1 ", ErrSyntax},
		{"111x", ErrSyntax},
		{"1.00.0000", ErrSyntax},
		{"999999999999999999999990", ErrRange},
		{"nAn", ErrSyntax},
		{"nAN", ErrSyntax},
		{"iNF", ErrSyntax},
		{"iNf", ErrSyntax},
		{"inF", ErrSyntax},
		{"+iNF", ErrSyntax},
		{"+iNf", ErrSyntax},
		{"+inF", ErrSyntax},
		{"-iNF", ErrSyntax},
		{"-iNf", ErrSyntax},
		{"-inF", ErrSyntax},
	}
	for _, c := range cases {
		suite.Run(fmt.Sprintf("case-%s", c.inp), func() {
			_, err := parse(c.inp)
			suite.Require().Error(err)
			suite.True(errors.Is(err, c.err))
		})
	}
}
