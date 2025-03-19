package decimal

import (
	"fmt"
	"math/big"
	"testing"

	"github.com/stretchr/testify/suite"
)

type convertersSuite struct {
	suite.Suite
	strConstructors
}

func TestConverters(t *testing.T) {
	suite.Run(t, new(convertersSuite))
}

func (suite *convertersSuite) TestFull() {
	cases := []string{
		// "0", "1", "-1", "0.0000000000001", "-0.0000000000001", "123456789.987654321", "123456789.987654321",
		"99999999999999999999999.999999999999999", "-99999999999999999999999.999999999999999",
	}

	for _, c := range cases {
		suite.Run(fmt.Sprintf("case-%s", c), func() {
			d := suite.decimal(c)
			f := suite.float(c)
			r := suite.rat(c)
			df := d.Float(nil)
			dr := d.Rat(nil)
			// This equality says that decimal and string parsing makes equally bad result
			suite.Zero(f.Cmp(df), "float: %38.5f", df)

			// Use this equality in your code
			suite.Zero(r.Cmp(dr), "rat: %s", dr.String())
		})
	}
}

type strConstructors struct{}

func (strConstructors) decimal(s string) Decimal128 {
	return Must(FromString(s))
}

func (strConstructors) float(s string) *big.Float {
	f, _ := (&big.Float{}).SetPrec(256).SetString(s)
	return f
}

func (strConstructors) rat(s string) *big.Rat {
	r, _ := (&big.Rat{}).SetString(s)
	return r
}
