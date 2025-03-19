package decimal

import (
	"encoding"
	"errors"
	"fmt"
	"testing"

	"github.com/stretchr/testify/suite"
)

var (
	_ fmt.Formatter            = Decimal128{}
	_ fmt.Scanner              = &Decimal128{}
	_ encoding.TextMarshaler   = Decimal128{}
	_ encoding.TextUnmarshaler = &Decimal128{}
)

type stringSuite struct {
	suite.Suite
}

func TestString(t *testing.T) {
	suite.Run(t, new(stringSuite))
}

func (suite *stringSuite) TestCreateFromInt() {
	cases := []struct {
		d                Decimal128
		wantPos, wantNeg string
	}{
		{Must(FromInt64(0)), "0", "0"},
		{Must(FromInt64(1)), "1", "-1"},
		{Must(NewFromFloat64(0.1)), "0.1", "-0.1"},
		{Must(NewFromFloat64(0.0001)), "0.0001", "-0.0001"},
		{Must(FromInt64(10000000000000000)), "10000000000000000", "-10000000000000000"},
		{decimalNan, "NaN", "NaN"},
		{decimalInf, "+Inf", "-Inf"},
	}
	for _, c := range cases {
		suite.Run(fmt.Sprintf("case-%s", c.wantPos), func() {
			gotPos := c.d.String()
			gotNeg := c.d.Neg().String()
			suite.Equal(c.wantPos, gotPos)
			suite.Equal(c.wantNeg, gotNeg)
		})
	}
}

type formatSuite struct {
	suite.Suite
}

func TestFormat(t *testing.T) {
	suite.Run(t, new(formatSuite))
}

func (suite *formatSuite) TestDefault() {
	cases := []struct {
		d                Decimal128
		wantPos, wantNeg string
	}{
		{Must(FromInt64(0)), "0", "0"},
		{Must(FromInt64(1)), "1", "-1"},
		{Must(NewFromFloat64(0.1)), "0.1", "-0.1"},
		{Must(NewFromFloat64(0.0001)), "0.0001", "-0.0001"},
		{Must(FromInt64(10000000000000000)), "10000000000000000", "-10000000000000000"},
		{decimalNan, "NaN", "NaN"},
		{decimalInf, "+Inf", "-Inf"},
	}
	for _, c := range cases {
		for _, f := range []string{"f", "F", "v"} {
			suite.Run(fmt.Sprintf("case-'%s'-%s", f, c.wantPos), func() {
				format := "%" + f
				gotPos := fmt.Sprintf(format, c.d)
				gotNeg := fmt.Sprintf(format, c.d.Neg())
				suite.Equal(c.wantPos, gotPos)
				suite.Equal(c.wantNeg, gotNeg)
			})
		}
	}
}

func (suite *formatSuite) TestUnsupported() {
	x := Must(FromInt64(1))
	for _, f := range []string{
		"t", "b", "c", "d", "o", "O", "q", "x", "X", "U", "b", "e", "E", "g", "G", "s", "q",
	} {
		suite.Run(fmt.Sprintf("case-'%s'", f), func() {
			format := "%" + f
			want := "%!" + f + "(decimal.Decimal128=1)"
			got := fmt.Sprintf(format, x)
			suite.Equal(want, got)
		})
	}
}

func (suite *formatSuite) TestFormatSign() {
	cases := []struct {
		d                Decimal128
		wantPos, wantNeg string
	}{
		{Must(FromInt64(0)), "+0", "+0"},
		{Must(FromInt64(1)), "+1", "-1"},
		{Must(NewFromFloat64(0.1)), "+0.1", "-0.1"},
		{Must(NewFromFloat64(0.0001)), "+0.0001", "-0.0001"},
		{Must(FromInt64(10000000000000000)), "+10000000000000000", "-10000000000000000"},
		{decimalNan, "NaN", "NaN"},
		{decimalInf, "+Inf", "-Inf"},
	}
	for _, c := range cases {
		for _, f := range []string{"f", "F", "v"} {
			suite.Run(fmt.Sprintf("case-'%s'-%s", f, c.wantPos), func() {
				format := "%+" + f
				gotPos := fmt.Sprintf(format, c.d)
				gotNeg := fmt.Sprintf(format, c.d.Neg())
				suite.Equal(c.wantPos, gotPos)
				suite.Equal(c.wantNeg, gotNeg)
			})
		}
	}
}

func (suite *formatSuite) TestFormatSpaceSign() {
	cases := []struct {
		d                Decimal128
		wantPos, wantNeg string
	}{
		{Must(FromInt64(0)), " 0", " 0"},
		{Must(FromInt64(1)), " 1", "-1"},
		{Must(NewFromFloat64(0.1)), " 0.1", "-0.1"},
		{Must(NewFromFloat64(0.0001)), " 0.0001", "-0.0001"},
		{Must(FromInt64(10000000000000000)), " 10000000000000000", "-10000000000000000"},
		{decimalNan, "NaN", "NaN"},
		{decimalInf, "+Inf", "-Inf"},
	}
	for _, c := range cases {
		for _, f := range []string{"f", "F", "v"} {
			suite.Run(fmt.Sprintf("case-'%s'-%s", f, c.wantPos), func() {
				format := "% " + f
				gotPos := fmt.Sprintf(format, c.d)
				gotNeg := fmt.Sprintf(format, c.d.Neg())
				suite.Equal(c.wantPos, gotPos)
				suite.Equal(c.wantNeg, gotNeg)
			})
		}
	}
}

func (suite *formatSuite) TestFormatPrec() {
	cases := []struct {
		d                Decimal128
		wantPos, wantNeg string
	}{
		{Must(FromInt64(0)), "0.000000000", "0.000000000"},
		{Must(FromInt64(1)), "1.000000000", "-1.000000000"},
		{Must(NewFromFloat64(0.1)), "0.100000000", "-0.100000000"},
		{Must(NewFromFloat64(0.0001)), "0.000100000", "-0.000100000"},
		{Must(NewFromFloat64(0.12345678912345)), "0.123456789", "-0.123456789"},
		{Must(FromInt64(10000000000000000)), "10000000000000000.000000000", "-10000000000000000.000000000"},
		{decimalNan, "NaN", "NaN"},
		{decimalInf, "+Inf", "-Inf"},
	}
	for _, c := range cases {
		for _, f := range []string{"f", "F", "v"} {
			suite.Run(fmt.Sprintf("case-'%s'-%s", f, c.wantPos), func() {
				format := "%.9" + f
				gotPos := fmt.Sprintf(format, c.d)
				gotNeg := fmt.Sprintf(format, c.d.Neg())
				suite.Equal(c.wantPos, gotPos)
				suite.Equal(c.wantNeg, gotNeg)
			})
		}
	}
}

func (suite *formatSuite) TestFormatPadLeft() {
	cases := []struct {
		d                Decimal128
		wantPos, wantNeg string
	}{
		{Must(FromInt64(0)), "        0", "        0"},
		{Must(FromInt64(1)), "        1", "       -1"},
		{Must(NewFromFloat64(0.1)), "      0.1", "     -0.1"},
		{Must(NewFromFloat64(0.0001)), "   0.0001", "  -0.0001"},
		{Must(FromInt64(10000000000000000)), "10000000000000000", "-10000000000000000"},
		{decimalNan, "      NaN", "      NaN"},
		{decimalInf, "     +Inf", "     -Inf"},
	}
	for _, c := range cases {
		for _, f := range []string{"f", "F", "v"} {
			suite.Run(fmt.Sprintf("case-'%s'-%s", f, c.wantPos), func() {
				format := "%9" + f
				gotPos := fmt.Sprintf(format, c.d)
				gotNeg := fmt.Sprintf(format, c.d.Neg())
				suite.Equal(c.wantPos, gotPos)
				suite.Equal(c.wantNeg, gotNeg)
			})
		}
	}
}

func (suite *formatSuite) TestFormatPadRight() {
	cases := []struct {
		d                Decimal128
		wantPos, wantNeg string
	}{
		{Must(FromInt64(0)), "0        ", "0        "},
		{Must(FromInt64(1)), "1        ", "-1       "},
		{Must(NewFromFloat64(0.1)), "0.1      ", "-0.1     "},
		{Must(NewFromFloat64(0.0001)), "0.0001   ", "-0.0001  "},
		{Must(FromInt64(10000000000000000)), "10000000000000000", "-10000000000000000"},
		{decimalNan, "NaN      ", "NaN      "},
		{decimalInf, "+Inf     ", "-Inf     "},
	}
	for _, c := range cases {
		for _, f := range []string{"f", "F", "v"} {
			suite.Run(fmt.Sprintf("case-'%s'-%s", f, c.wantPos), func() {
				format := "%-9" + f
				gotPos := fmt.Sprintf(format, c.d)
				gotNeg := fmt.Sprintf(format, c.d.Neg())
				suite.Equal(c.wantPos, gotPos)
				suite.Equal(c.wantNeg, gotNeg)
			})
		}
	}
}

func (suite *formatSuite) TestFormatPadZero() {
	cases := []struct {
		d                Decimal128
		wantPos, wantNeg string
	}{
		{Must(FromInt64(0)), "000000000", "000000000"},
		{Must(FromInt64(1)), "000000001", "-00000001"},
		{Must(NewFromFloat64(0.1)), "0000000.1", "-000000.1"},
		{Must(NewFromFloat64(0.0001)), "0000.0001", "-000.0001"},
		{Must(FromInt64(10000000000000000)), "10000000000000000", "-10000000000000000"},
		{decimalNan, "      NaN", "      NaN"},
		{decimalInf, "     +Inf", "     -Inf"},
	}
	for _, c := range cases {
		for _, f := range []string{"f", "F", "v"} {
			suite.Run(fmt.Sprintf("case-'%s'-%s", f, c.wantPos), func() {
				format := "%09" + f
				gotPos := fmt.Sprintf(format, c.d)
				gotNeg := fmt.Sprintf(format, c.d.Neg())
				suite.Equal(c.wantPos, gotPos)
				suite.Equal(c.wantNeg, gotNeg)
			})
		}
	}
}

func (suite *stringSuite) TestScan() {
	var d Decimal128
	n, err := fmt.Sscan("123456", &d)
	suite.Require().NoError(err)
	suite.EqualValues(1, n)
	suite.Equal("123456", d.String())
}

func (suite *stringSuite) TestScanError() {
	var d Decimal128
	_, err := fmt.Sscan("x", &d)
	suite.Require().Error(err)
	suite.True(errors.Is(err, ErrScan), "actual: %#v", err)
	suite.True(errors.Is(err, ErrSyntax), "actual: %#v", err)
}

func (suite *stringSuite) TestScanRuneError() {
	var d Decimal128
	_, err := fmt.Sscan("Я", &d)
	suite.Require().Error(err)
	suite.True(errors.Is(err, ErrScan), "actual: %#v", err)
}

func (suite *stringSuite) TestScanFormats() {
	var d Decimal128
	for _, f := range []string{
		"f", "s", "v",
	} {
		suite.Run(fmt.Sprintf("case-'%s'", f), func() {
			format := "%" + f
			_, err := fmt.Sscanf("1", format, &d)
			suite.NoError(err)
		})
	}

	for _, f := range []string{
		"t", "b", "c", "d", "o", "O", "q", "x", "X", "U", "b", "e", "E", "g", "G", "q",
	} {
		suite.Run(fmt.Sprintf("case-'%s'", f), func() {
			format := "%" + f
			_, err := fmt.Sscanf("1", format, &d)
			suite.Require().Error(err)
			suite.True(errors.Is(err, ErrScan), "actual: %#v", err)
		})
	}
}

func (suite *stringSuite) TestMarshalText() {
	b, err := Must(FromInt64(1)).MarshalText()
	suite.Require().NoError(err)
	suite.EqualValues("1", b)
}

func (suite *stringSuite) TestUnmarshalText() {
	var d Decimal128
	err := d.UnmarshalText([]byte("1"))
	suite.Require().NoError(err)
	suite.Equal("1", d.String())
}
