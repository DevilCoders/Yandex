package decimal

import (
	"encoding/json"
	"errors"
	"fmt"
	"testing"

	"github.com/stretchr/testify/suite"
	"gopkg.in/yaml.v2"
)

var (
	_ json.Marshaler   = Decimal128{}
	_ json.Unmarshaler = &Decimal128{}
	_ yaml.Unmarshaler = &Decimal128{}
)

type jsonSuite struct {
	suite.Suite
	strConstructors
}

func TestJSON(t *testing.T) {
	suite.Run(t, new(jsonSuite))
}

func (suite *jsonSuite) TestMarshal() {
	cases := []struct {
		x    Decimal128
		want string
	}{
		{suite.decimal("0"), "0"},
		{suite.decimal("1.234567"), "1.234567"},
		{suite.decimal("123456789.012345"), "123456789.012345"},
		{suite.decimal("123456789012345"), "123456789012345"},
		{suite.decimal("-123456789012345"), "-123456789012345"},
		{suite.decimal("123456789.0123456"), `"123456789.0123456"`},
		{suite.decimal("1234567890123456"), `"1234567890123456"`},
	}
	for _, c := range cases {
		suite.Run(fmt.Sprintf("case-%s", c.want), func() {
			b, err := json.Marshal(c.x)
			suite.Require().NoError(err)
			suite.EqualValues(c.want, b)
		})
	}
}

func (suite *jsonSuite) TestMarshalNan() {
	_, err := json.Marshal(suite.decimal("NaN"))
	suite.Require().Error(err)
	suite.True(errors.Is(err, ErrNan), "actual: %#v", err)
}

func (suite *jsonSuite) TestUnmarshal() {
	cases := []struct {
		inp  string
		want Decimal128
	}{
		{"0", suite.decimal("0")},
		{"1.234567", suite.decimal("1.234567")},
		{"123456789.012345", suite.decimal("123456789.012345")},
		{"-123456789.012345", suite.decimal("-123456789.012345")},
		{"99999999999999999999999.999999999999999", suite.decimal("99999999999999999999999.999999999999999")},
	}
	for _, c := range cases {
		suite.Run(fmt.Sprintf("case-%s", c.inp), func() {
			var got, gotQuoted Decimal128
			suite.Require().NoError(json.Unmarshal([]byte(c.inp), &got))
			suite.Require().NoError(json.Unmarshal([]byte(`"`+c.inp+`"`), &gotQuoted))

			suite.Zero(c.want.Cmp(got), "simple: %s", got.String())
			suite.Zero(c.want.Cmp(gotQuoted), "quoted: %s", gotQuoted.String())
		})
	}
}

func (suite *jsonSuite) TestUnmarshalNan() {
	var d Decimal128
	err := json.Unmarshal([]byte(`"NaN"`), &d)
	suite.Require().Error(err)
	suite.True(errors.Is(err, ErrNan), "actual: %#v", err)
}

type yamlSuite struct {
	suite.Suite
	strConstructors
}

func TestYAML(t *testing.T) {
	suite.Run(t, new(yamlSuite))
}

func (suite *yamlSuite) TestMarshal() {
	cases := []struct {
		x    Decimal128
		want string
	}{
		{suite.decimal("0"), `"0"`},
		{suite.decimal("1.234567"), `"1.234567"`},
		{suite.decimal("123456789.012345"), `"123456789.012345"`},
		{suite.decimal("NaN"), `NaN`},
	}
	for _, c := range cases {
		suite.Run(fmt.Sprintf("case-%s", c.want), func() {
			b, err := yaml.Marshal(c.x)
			suite.Require().NoError(err)
			suite.EqualValues(c.want+"\n", b)
		})
	}
}

func (suite *yamlSuite) TestUnmarshal() {
	cases := []struct {
		inp  string
		want Decimal128
	}{
		{"0", suite.decimal("0")},
		{"1.234567", suite.decimal("1.234567")},
		{"123456789.012345", suite.decimal("123456789.012345")},
		{"-123456789.012345", suite.decimal("-123456789.012345")},
		{"99999999999999999999999.999999999999999", suite.decimal("99999999999999999999999.999999999999999")},
		{"NaN", suite.decimal("NaN")},
	}
	for _, c := range cases {
		suite.Run(fmt.Sprintf("case-%s", c.inp), func() {
			var got, gotQuoted Decimal128
			suite.Require().NoError(yaml.Unmarshal([]byte(c.inp), &got))
			suite.Require().NoError(yaml.Unmarshal([]byte(`"`+c.inp+`"`), &gotQuoted))

			suite.Zero(c.want.Cmp(got), "simple: %s", got.String())
			suite.Zero(c.want.Cmp(gotQuoted), "quoted: %s", gotQuoted.String())
		})
	}
}
