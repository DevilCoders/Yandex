package skuresolve

import (
	"testing"

	"github.com/stretchr/testify/suite"
)

type skuMatcherTestSuite struct {
	suite.Suite
	metric  string
	getter  MetricValueGetter
	matcher Matcher
}

func TestSkuMatcher(t *testing.T) {
	suite.Run(t, new(skuMatcherTestSuite))
}

func (suite *skuMatcherTestSuite) SetupTest() {
	suite.metric = `
{
	"schema": "schema1",
	"version": "version",
	"usage":{
		"quantity": 2,
		"start": 999,
		"finish": 1000,
		"unit": "unit",
		"type": "delta"
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
	suite.matcher = NewMatcher()
}

func (suite *skuMatcherTestSuite) TestEmptyMatch() {
	suite.matcher.AddResolvers("schema1")

	skus, err := suite.matcher.Match("schema1", suite.getter)
	suite.Require().NoError(err)
	suite.Empty(skus)
}

func (suite *skuMatcherTestSuite) TestMatchRules() {
	r, err := NewRulesResolver("sku1", PathsMatcher{"tags.exists": NewExistsMatcher(true)})
	suite.Require().NoError(err)
	suite.matcher.AddResolvers("schema1", r)

	r, err = NewRulesResolver("sku2", PathsMatcher{"tags.notexists": NewExistsMatcher(false)})
	suite.Require().NoError(err)
	suite.matcher.AddResolvers("schema1", r)

	r, err = NewRulesResolver("sku3", PathsMatcher{
		"schema":     NewStringMatcher("schema1"),
		"version":    NewStringMatcher("version"),
		"usage.type": NewStringMatcher("delta"),
		"usage.unit": NewStringMatcher("unit"),
	})
	suite.Require().NoError(err)
	suite.matcher.AddResolvers("schema1", r)

	r, err = NewRulesResolver("sku that not match", PathsMatcher{"tags.notexists": NewExistsMatcher(true)})
	suite.Require().NoError(err)
	suite.matcher.AddResolvers("schema1", r)

	skus, err := suite.matcher.Match("schema1", suite.getter)
	suite.Require().NoError(err)
	suite.ElementsMatch([]string{"sku1", "sku2", "sku3"}, skus)
}

func (suite *skuMatcherTestSuite) TestMatchPolicy() {
	r, err := NewPolicyResolver("sku1", "tags.exists")
	suite.Require().NoError(err)
	suite.matcher.AddResolvers("schema1", r)

	r, err = NewPolicyResolver("sku2", "!(tags.notexists)")
	suite.Require().NoError(err)
	suite.matcher.AddResolvers("schema1", r)

	r, err = NewPolicyResolver("sku3", "schema=='schema1'&&version=='version'&&usage.type=='delta'&&usage.unit=='unit'")
	suite.Require().NoError(err)
	suite.matcher.AddResolvers("schema1", r)

	r, err = NewPolicyResolver("sku_static", "`true`")
	suite.Require().NoError(err)
	suite.matcher.AddResolvers("schema1", r)

	r, err = NewPolicyResolver("sku that not match", "tags.notexists")
	suite.Require().NoError(err)
	suite.matcher.AddResolvers("schema1", r)

	r, err = NewPolicyResolver("sku with emty policy", "")
	suite.Require().NoError(err)
	suite.matcher.AddResolvers("schema1", r)

	skus, err := suite.matcher.Match("schema1", suite.getter)
	suite.Require().NoError(err)
	suite.ElementsMatch([]string{"sku1", "sku2", "sku3", "sku_static"}, skus)
}

func (suite *skuMatcherTestSuite) TestPolicySyntaxError() {
	_, err := NewPolicyResolver("sku1", "'")
	suite.Require().Error(err)
}

func (suite *skuMatcherTestSuite) TestPolicyExecError() {
	r, err := NewPolicyResolver("sku1", "add('type error', tags.exists)")
	suite.Require().NoError(err)
	suite.matcher.AddResolvers("schema1", r)

	_, err = suite.matcher.Match("schema1", suite.getter)
	suite.Require().Error(err)
}

func (suite *skuMatcherTestSuite) TestNoSchema() {
	r, err := NewPolicyResolver("sku_static", "`true`")
	suite.Require().NoError(err)
	suite.matcher.AddResolvers("schema1", r)

	skus, err := suite.matcher.Match("schema2", suite.getter)
	suite.Require().NoError(err)
	suite.Empty(skus)
}

func (suite *skuMatcherTestSuite) TestNotMatchRules() {
	r, err := NewPolicyResolver("sku1", "!(tags.exists)")
	suite.Require().NoError(err)
	suite.matcher.AddResolvers("schema1", r)

	r, err = NewRulesResolver("sku2", PathsMatcher{"tags.exists": NewExistsMatcher(false)})
	suite.Require().NoError(err)
	suite.matcher.AddResolvers("schema1", r)

	skus, err := suite.matcher.Match("schema1", suite.getter)
	suite.Require().NoError(err)
	suite.Empty(skus)
}
