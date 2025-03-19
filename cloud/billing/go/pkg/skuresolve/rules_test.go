package skuresolve

import (
	"fmt"
	"testing"
	"time"

	"github.com/stretchr/testify/suite"
	"github.com/valyala/fastjson"

	"a.yandex-team.ru/cloud/billing/go/pkg/decimal"
)

type resolvingRuleMatchersTestSuite struct {
	suite.Suite
	metric string
	getter MetricValueGetter
}

func TestResolvingRuleMatchers(t *testing.T) {
	suite.Run(t, new(resolvingRuleMatchersTestSuite))
}

func (suite *resolvingRuleMatchersTestSuite) SetupTest() {
	suite.metric = `
{
	"usage":{
		"quantity": 2,
		"finish": 1000
	},
	"tags":{
		"exists": true,
		"key": "value"
	}
}
`
	fg, err := ParseToFastJSONGetter(suite.metric)
	suite.Require().NoError(err)
	suite.getter = fg
}

func (suite *resolvingRuleMatchersTestSuite) TestRuleMatch() {
	m := PathsMatcher{
		"tags.notexists": NewExistsMatcher(false),
		"tags.exists":    NewExistsMatcher(true),
		"tags.key":       NewStringMatcher("value"),
	}

	suite.True(m.ValueMatches(suite.getter))
}

func (suite *resolvingRuleMatchersTestSuite) TestRuleNotMatch() {
	m := PathsMatcher{
		"tags.exists": NewExistsMatcher(false),
		"tags.key":    NewStringMatcher("value"),
	}

	suite.False(m.ValueMatches(suite.getter))
}

func (suite *resolvingRuleMatchersTestSuite) TestOneRuleMatch() {
	m := orRulesMatcher{
		PathsMatcher{"tags.exists": NewExistsMatcher(false)},
		PathsMatcher{"tags.key": NewStringMatcher("value")},
	}

	suite.True(m.ValueMatches(suite.getter))
}

func (suite *resolvingRuleMatchersTestSuite) TestNoneRulesMatch() {
	m := orRulesMatcher{
		PathsMatcher{"tags.exists": NewExistsMatcher(false)},
		PathsMatcher{"tags.key": NewStringMatcher("")},
	}

	suite.False(m.ValueMatches(suite.getter))
}

type valueMatcherTestSuite struct {
	suite.Suite
	json *fastjson.Value
}

func TestValueMatcher(t *testing.T) {
	suite.Run(t, new(valueMatcherTestSuite))
}

func (suite *valueMatcherTestSuite) SetupSuite() {
	suite.json = fastjson.MustParse(`{
		"null": null,
		"empty": "",
		"string": "string",
		"number": 123.456,
		"number_str": "123.456",
		"true": true,
		"false": false
	}`)
}

func (suite *valueMatcherTestSuite) TestNull() {
	matcher := NewNullMatcher()

	suite.True(matcher.MatchJSON(suite.json.Get("null")))
	suite.False(matcher.MatchJSON(suite.json.Get("empty")))
	suite.False(matcher.MatchJSON(suite.json.Get("string")))
	suite.False(matcher.MatchJSON(suite.json.Get("number")))
	suite.False(matcher.MatchJSON(suite.json.Get("number_str")))
	suite.False(matcher.MatchJSON(suite.json.Get("true")))
	suite.False(matcher.MatchJSON(suite.json.Get("false")))
	suite.True(matcher.MatchJSON(suite.json.Get("notexists")))
}

func (suite *valueMatcherTestSuite) TestEmptyString() {
	matcher := NewStringMatcher("")

	suite.False(matcher.MatchJSON(suite.json.Get("null")))
	suite.True(matcher.MatchJSON(suite.json.Get("empty")))
	suite.False(matcher.MatchJSON(suite.json.Get("string")))
	suite.False(matcher.MatchJSON(suite.json.Get("number")))
	suite.False(matcher.MatchJSON(suite.json.Get("number_str")))
	suite.False(matcher.MatchJSON(suite.json.Get("true")))
	suite.False(matcher.MatchJSON(suite.json.Get("false")))
	suite.False(matcher.MatchJSON(suite.json.Get("notexists")))
}

func (suite *valueMatcherTestSuite) TestString() {
	matcher := NewStringMatcher("string")

	suite.False(matcher.MatchJSON(suite.json.Get("null")))
	suite.False(matcher.MatchJSON(suite.json.Get("empty")))
	suite.True(matcher.MatchJSON(suite.json.Get("string")))
	suite.False(matcher.MatchJSON(suite.json.Get("number")))
	suite.False(matcher.MatchJSON(suite.json.Get("number_str")))
	suite.False(matcher.MatchJSON(suite.json.Get("true")))
	suite.False(matcher.MatchJSON(suite.json.Get("false")))
	suite.False(matcher.MatchJSON(suite.json.Get("notexists")))
}

func (suite *valueMatcherTestSuite) TestNumber() {
	matcher := NewNumberMatcher(decimal.Must(decimal.FromString("123.456")))

	suite.False(matcher.MatchJSON(suite.json.Get("null")))
	suite.False(matcher.MatchJSON(suite.json.Get("empty")))
	suite.False(matcher.MatchJSON(suite.json.Get("string")))
	suite.True(matcher.MatchJSON(suite.json.Get("number")))
	suite.True(matcher.MatchJSON(suite.json.Get("number_str")))
	suite.False(matcher.MatchJSON(suite.json.Get("true")))
	suite.False(matcher.MatchJSON(suite.json.Get("false")))
	suite.False(matcher.MatchJSON(suite.json.Get("notexists")))
}

func (suite *valueMatcherTestSuite) TestTrue() {
	matcher := NewBoolMatcher(true)

	suite.False(matcher.MatchJSON(suite.json.Get("null")))
	suite.False(matcher.MatchJSON(suite.json.Get("empty")))
	suite.False(matcher.MatchJSON(suite.json.Get("string")))
	suite.False(matcher.MatchJSON(suite.json.Get("number")))
	suite.False(matcher.MatchJSON(suite.json.Get("number_str")))
	suite.True(matcher.MatchJSON(suite.json.Get("true")))
	suite.False(matcher.MatchJSON(suite.json.Get("false")))
	suite.False(matcher.MatchJSON(suite.json.Get("notexists")))
}

func (suite *valueMatcherTestSuite) TestFalse() {
	matcher := NewBoolMatcher(false)

	suite.False(matcher.MatchJSON(suite.json.Get("null")))
	suite.False(matcher.MatchJSON(suite.json.Get("empty")))
	suite.False(matcher.MatchJSON(suite.json.Get("string")))
	suite.False(matcher.MatchJSON(suite.json.Get("number")))
	suite.False(matcher.MatchJSON(suite.json.Get("number_str")))
	suite.False(matcher.MatchJSON(suite.json.Get("true")))
	suite.True(matcher.MatchJSON(suite.json.Get("false")))
	suite.False(matcher.MatchJSON(suite.json.Get("notexists")))
}

func (suite *valueMatcherTestSuite) TestValue() {
	cases := []struct {
		value decimal.Decimal128
		want  bool
	}{
		{decimal.Must(decimal.FromString("0")), false},
		{decimal.Must(decimal.FromString("123.456")), true},
		{decimal.Must(decimal.FromString("NaN")), false},
		{decimal.Must(decimal.FromString("Inf")), false},
		{decimal.Must(decimal.FromString("-Inf")), false},
	}

	stringCases := []struct {
		value string
		want  bool
	}{
		{"string", true},
		{"other string", false},
	}

	matcher := NewNumberMatcher(decimal.Must(decimal.FromString("123.456")))
	stringMatcher := NewStringMatcher("string")
	otherMatchers := []MetricValueMatcher{
		NewNullMatcher(),
		NewStringMatcher(""),
		NewBoolMatcher(true),
		NewBoolMatcher(false),
	}

	for _, c := range cases {
		suite.Run(fmt.Sprintf("case-%f", c.value), func() {
			got := matcher.MatchValue(c.value)
			suite.EqualValues(c.want, got)

			suite.False(stringMatcher.MatchValue(c.value))
			for _, om := range otherMatchers {
				suite.False(om.MatchValue(c.value))
			}
		})
	}

	for _, c := range stringCases {
		suite.Run(fmt.Sprintf("strcase-%s", c.value), func() {
			got := stringMatcher.MatchValue(c.value)
			suite.EqualValues(c.want, got)

			suite.False(matcher.MatchValue(c.value))
			for _, om := range otherMatchers {
				suite.False(om.MatchValue(c.value))
			}
		})
	}

	for _, matcher := range append(otherMatchers, matcher) {
		suite.Require().False(matcher.MatchValue("string"))
		suite.Require().False(matcher.MatchValue(nil))
		suite.Require().False(matcher.MatchValue(1))
		suite.Require().False(matcher.MatchValue(1.1))
		suite.Require().False(matcher.MatchValue(true))
	}
}

type existsMatcherTestSuite struct {
	suite.Suite
}

func TestExistsMatcher(t *testing.T) {
	suite.Run(t, new(existsMatcherTestSuite))
}

func (suite *existsMatcherTestSuite) TestExistsJSON() {
	json := fastjson.MustParse(`{"exists": true}`)

	matcher := NewExistsMatcher(true)
	notMatcher := NewExistsMatcher(false)

	suite.Require().True(matcher.MatchJSON(json.Get("exists")))
	suite.Require().False(matcher.MatchJSON(json.Get("notexists")))

	suite.Require().True(notMatcher.MatchJSON(json.Get("notexists")))
	suite.Require().False(notMatcher.MatchJSON(json.Get("exists")))
}

func (suite *existsMatcherTestSuite) TestExistsDecimalValue() {
	cases := []struct {
		value decimal.Decimal128
		want  bool
	}{
		{decimal.Must(decimal.FromString("0")), false},
		{decimal.Must(decimal.FromString("1")), true},
		{decimal.Must(decimal.FromString("NaN")), true},
		{decimal.Must(decimal.FromString("Inf")), true},
		{decimal.Must(decimal.FromString("-Inf")), true},
	}

	matcher := NewExistsMatcher(true)
	notMatcher := NewExistsMatcher(false)

	for _, c := range cases {
		suite.Run(fmt.Sprintf("case-%f", c.value), func() {
			got := matcher.MatchValue(c.value)
			notgot := notMatcher.MatchValue(c.value)

			suite.EqualValues(c.want, got)
			suite.EqualValues(!c.want, notgot)
		})
	}

	suite.Require().False(matcher.MatchValue("string"))
	suite.Require().False(matcher.MatchValue(nil))
	suite.Require().False(matcher.MatchValue(1))
	suite.Require().False(matcher.MatchValue(1.1))
	suite.Require().False(matcher.MatchValue(true))
}

func (suite *existsMatcherTestSuite) TestExistsTimeValue() {
	cases := []struct {
		value time.Time
		want  bool
	}{
		{time.Time{}, false},
		{time.Unix(0, 0), false},
		{time.Unix(0, 1), false},
		{time.Now(), true},
	}

	matcher := NewExistsMatcher(true)
	notMatcher := NewExistsMatcher(false)

	for _, c := range cases {
		suite.Run(fmt.Sprintf("case-%s", c.value.String()), func() {
			got := matcher.MatchValue(c.value)
			notgot := notMatcher.MatchValue(c.value)

			suite.EqualValues(c.want, got)
			suite.EqualValues(!c.want, notgot)
		})
	}

	suite.Require().False(matcher.MatchValue("string"))
	suite.Require().False(matcher.MatchValue(nil))
	suite.Require().False(matcher.MatchValue(1))
	suite.Require().False(matcher.MatchValue(1.1))
	suite.Require().False(matcher.MatchValue(true))
}
