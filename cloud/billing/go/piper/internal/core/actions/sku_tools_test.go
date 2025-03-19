package actions

import (
	"testing"
	"time"

	"github.com/stretchr/testify/suite"
	"github.com/valyala/fastjson"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/core/entities"
	"a.yandex-team.ru/cloud/billing/go/pkg/decimal"
)

type metricValueGetterTestSuite struct {
	suite.Suite
	metric entities.SourceMetric
	match  bool

	matchValue string
}

func TestMetricValueGetter(t *testing.T) {
	suite.Run(t, new(metricValueGetterTestSuite))
}

func (suite *metricValueGetterTestSuite) SetupTest() {
	suite.metric = entities.SourceMetric{}
	suite.metric.Usage.Quantity, _ = decimal.FromInt64(1)
	suite.metric.Usage.Finish = time.Unix(1000, 0)
	suite.metric.Tags = `{
		"value": "test me",
		"object": {"value": "from object", "list":[null, "from array", false]}
	}`
	suite.match = true
	suite.matchValue = ""
}

func (suite *metricValueGetterTestSuite) MatchValue(v interface{}) bool {
	switch vv := v.(type) {
	case decimal.Decimal128:
		if vv.Cmp(suite.metric.Usage.Quantity) != 0 {
			panic("wrong match value")
		}
	case int64:
		if vv != suite.metric.Usage.Finish.Unix() {
			panic("wrong match value")
		}
	}
	return suite.match
}

func (suite *metricValueGetterTestSuite) MatchJSON(v *fastjson.Value) bool {
	if v != nil {
		suite.matchValue = v.String()
	} else {
		suite.matchValue = ""
	}
	return suite.match
}

func (suite *metricValueGetterTestSuite) TestMatchQuantity() {
	getter := metricValueGetter{metric: &suite.metric}

	suite.match = true
	suite.Require().True(getter.MatchByPath("usage.quantity", suite))
	suite.match = false
	suite.Require().False(getter.MatchByPath("usage.quantity", suite))

	suite.match = true
	suite.Require().False(getter.MatchByPath("something.else", suite))
}

func (suite *metricValueGetterTestSuite) TestMatchFinish() {
	getter := metricValueGetter{metric: &suite.metric}

	suite.match = true
	suite.Require().True(getter.MatchByPath("usage.finish", suite))
	suite.match = false
	suite.Require().False(getter.MatchByPath("usage.finish", suite))

	suite.match = true
	suite.Require().False(getter.MatchByPath("something.else", suite))
}

func (suite *metricValueGetterTestSuite) TestMatchTag() {
	cases := []struct {
		path string
		want string
	}{
		{"tags.value", `"test me"`},
		{"tags.object.value", `"from object"`},
		{"tags.object.list[1]", `"from array"`},
		{"tags.other", ""},
	}

	getter := metricValueGetter{metric: &suite.metric}

	for _, c := range cases {
		suite.Run(c.path, func() {
			suite.matchValue = ""
			suite.Require().True(getter.MatchByPath(c.path, suite))
			suite.EqualValues(c.want, suite.matchValue)
		})
	}
}

func (suite *metricValueGetterTestSuite) TestNoTag() {
	suite.metric.Tags = ""
	getter := metricValueGetter{metric: &suite.metric}

	suite.Require().NotPanics(func() {
		_, _ = getter.MatchByPath("tags.value", suite)
	})
}

func (suite *metricValueGetterTestSuite) TestFullValue() {
	getter := metricValueGetter{metric: &suite.metric}

	value := getter.GetFullValue()

	suite.Equal(1, value.GetInt("usage", "quantity"))
	suite.Equal(1000, value.GetInt("usage", "finish"))
	suite.EqualValues("test me", value.GetStringBytes("tags", "value"))
}

func (suite *metricValueGetterTestSuite) TestReset() {
	getter := metricValueGetter{metric: &suite.metric}
	_, _ = getter.MatchByPath("tags.value", suite)

	suite.Require().NotNil(getter.parser)
	suite.Require().NotNil(getter.arena)
	suite.Require().NotNil(getter.tags)
	suite.Require().NotNil(getter.fullValue)

	getter.reset()
	suite.Require().Nil(getter.parser)
	suite.Require().Nil(getter.arena)
	suite.Require().Nil(getter.tags)
	suite.Require().Nil(getter.fullValue)

	suite.Require().NotPanics(func() {
		getter.reset()
	})
}

type metricValueGetterVariantsTestSuite struct {
	suite.Suite
	getter     metricValueGetter
	metric     entities.SourceMetric
	matchValue interface{}
}

func TestMetricValueGetterVariants(t *testing.T) {
	suite.Run(t, new(metricValueGetterVariantsTestSuite))
}

func (suite *metricValueGetterVariantsTestSuite) SetupTest() {
}

func (suite *metricValueGetterVariantsTestSuite) MatchValue(v interface{}) bool {
	suite.matchValue = v
	return true
}

func (suite *metricValueGetterVariantsTestSuite) MatchJSON(v *fastjson.Value) bool {
	// if v != nil {
	// 	suite.matchValue = v.String()
	// } else {
	// 	suite.matchValue = ""
	// }
	return true
}

func (suite *metricValueGetterVariantsTestSuite) reset() {
	suite.metric = entities.SourceMetric{}
	suite.getter = metricValueGetter{metric: &suite.metric}
	suite.matchValue = nil
}

func (suite *metricValueGetterVariantsTestSuite) matchPath(path string) {
	_, err := suite.getter.MatchByPath(path, suite)
	suite.Require().NoError(err)
}

func (suite *metricValueGetterVariantsTestSuite) TestValues() {
	dec, _ := decimal.FromInt64(42)
	ts := time.Unix(1000, 0)
	tsInt := ts.Unix()

	suite.reset()
	suite.metric.Usage.Quantity = dec
	suite.matchPath("usage.quantity")
	suite.Equal(dec, suite.matchValue)

	suite.reset()
	suite.metric.Usage.Start = ts
	suite.matchPath("usage.start")
	suite.Equal(tsInt, suite.matchValue)

	suite.reset()
	suite.metric.Usage.Finish = ts
	suite.matchPath("usage.finish")
	suite.Equal(tsInt, suite.matchValue)

	suite.reset()
	suite.metric.Usage.Unit = "unit"
	suite.matchPath("usage.unit")
	suite.Equal("unit", suite.matchValue)

	suite.reset()
	suite.metric.Usage.RawType = "delta"
	suite.matchPath("usage.type")
	suite.Equal("delta", suite.matchValue)

	suite.reset()
	suite.metric.Schema = "schema"
	suite.matchPath("schema")
	suite.Equal("schema", suite.matchValue)

	suite.reset()
	suite.metric.Version = "version"
	suite.matchPath("version")
	suite.Equal("version", suite.matchValue)
}
