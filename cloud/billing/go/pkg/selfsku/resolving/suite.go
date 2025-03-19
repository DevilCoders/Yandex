package resolving

import (
	"encoding/json"
	"io/fs"

	"github.com/stretchr/testify/suite"

	"a.yandex-team.ru/cloud/billing/go/pkg/decimal"
	"a.yandex-team.ru/cloud/billing/go/pkg/selfsku/bundler"
	"a.yandex-team.ru/cloud/billing/go/pkg/selfsku/origin"
	"a.yandex-team.ru/cloud/billing/go/pkg/selfsku/tools"
	"a.yandex-team.ru/cloud/billing/go/pkg/skuresolve"
)

type unitsPair [2]string

type ResolvingTestSuite struct {
	suite.Suite
	DataDir fs.FS

	cases   []TestCase
	skus    map[string]origin.Sku
	units   map[unitsPair]origin.UnitConversion
	schemas map[string]origin.SchemaTagChecks

	matcher skuresolve.Matcher
}

func (suite *ResolvingTestSuite) SetupSuite() {
	schemas, err := bundler.LoadSchemas(suite.DataDir)
	suite.Require().NoError(err)

	suite.schemas = make(map[string]origin.SchemaTagChecks)
	for _, bndl := range schemas {
		for schema, tagCheck := range bndl.TagChecks {
			suite.schemas[schema] = tagCheck
		}
	}

	units, err := bundler.LoadUnits(suite.DataDir)
	suite.Require().NoError(err)

	suite.units = make(map[unitsPair]origin.UnitConversion)
	for _, bndl := range units {
		suite.units[unitsPair{bndl.SrcUnit, bndl.DstUnit}] = bndl.UnitConversion
	}

	serviceSkus, err := bundler.LoadSkus(suite.DataDir)
	suite.Require().NoError(err)

	suite.skus = make(map[string]origin.Sku)
	for _, bndl := range serviceSkus {
		for name, sku := range bndl.Skus {
			suite.skus[name] = sku
		}
	}

	cases, err := LoadResolvingCases(suite.DataDir)
	suite.Require().NoError(err)
	suite.cases = cases

	suite.setupResolvers()
}

func (suite *ResolvingTestSuite) TestResolving() {
	for _, c := range suite.cases {
		suite.Run(c.LoadedFrom(), func() {
			metricGetter := suite.makeGetter(c.Metric)
			skuIDs, err := suite.matcher.Match(c.Metric.Schema, metricGetter)
			suite.Require().NoError(err)

			skus := tools.NewStringSet()
			results := map[string]SkuResult{}
			for _, id := range skuIDs {
				sku, found := suite.skus[id]
				if !found {
					suite.Fail(id, "unknown sku in resolving")
					continue
				}

				result := SkuResult{
					Usage: ResultQuantity{
						Quantity: c.Metric.Usage.Quantity,
						Unit:     sku.Units.Usage,
					},
				}
				result.Pricing = suite.calculatePricing(id, sku, result.Usage, metricGetter)
				skus.Add(id)
				results[id] = result
			}

			for id, wantResult := range c.Skus {
				if !skus.Contains(id) {
					suite.Fail(id, "wanted sku not in resolve result")
					continue
				}
				suite.EqualValues(wantResult, results[id], id)
				skus.Remove(id)
			}
			suite.Require().Empty(skus.Items())
		})
	}
}

func (suite *ResolvingTestSuite) TestTags() {
	for _, c := range suite.cases {
		suite.Run(c.LoadedFrom(), func() {
			tagChecks := suite.schemas[c.Metric.Schema]
			req := tools.NewStringSet(tagChecks.Required...)
			for t := range c.Metric.Tags {
				req.Remove(t)
			}
			suite.Require().Empty(req.Items(), "required tags not found")
		})
	}
}

func (suite *ResolvingTestSuite) setupResolvers() {
	suite.matcher = skuresolve.NewMatcher()
	for name, sku := range suite.skus {
		resolvers := []skuresolve.Resolver{}
		switch {
		case len(sku.ResolvingRules) == 0:
			policy := "`true`"
			if len(sku.ResolvingPolicy) != 0 {
				policy = sku.ResolvingPolicy
			}
			resolver, err := skuresolve.NewPolicyResolver(name, skuresolve.JMESPath(policy))
			suite.Require().NoError(err)
			resolvers = append(resolvers, resolver)
		default:
			pms := []skuresolve.PathsMatcher{}
			for _, pathRule := range sku.ResolvingRules {
				m := skuresolve.PathsMatcher{}
				for path, rule := range pathRule {
					switch {
					case rule.ExistsRule:
						m[path] = skuresolve.NewExistsMatcher(rule.ExistsValue)
					case rule.Null:
						m[path] = skuresolve.NewNullMatcher()
					case rule.EmptyString:
						m[path] = skuresolve.NewStringMatcher("")
					case rule.Str != "":
						m[path] = skuresolve.NewStringMatcher(rule.Str)
					case rule.IsTrue:
						m[path] = skuresolve.NewBoolMatcher(true)
					case rule.IsFalse:
						m[path] = skuresolve.NewBoolMatcher(false)
					default:
						val := decimal.Must(decimal.NewFromFloat64(rule.Number))
						m[path] = skuresolve.NewNumberMatcher(val)
					}
				}
				pms = append(pms, m)
			}
			resolver, err := skuresolve.NewRulesResolver(name, pms...)
			suite.Require().NoError(err)
			resolvers = append(resolvers, resolver)
		}

		for _, sch := range sku.Schemas {
			suite.matcher.AddResolvers(sch, resolvers...)
		}
	}
}

func (suite *ResolvingTestSuite) makeGetter(metric TestMetric) *skuresolve.FastJSONGetter {
	jsonValue, err := json.Marshal(metric)
	suite.Require().NoError(err)

	mg, err := skuresolve.ParseToFastJSONGetter(string(jsonValue))
	suite.Require().NoError(err)
	return mg
}

func (suite *ResolvingTestSuite) calculatePricing(
	skuName string, sku origin.Sku, usage ResultQuantity, mg skuresolve.MetricValueGetter,
) (pricing ResultQuantity) {
	pricing.Unit = sku.Units.Pricing
	pricing.Quantity = usage.Quantity

	if sku.PricingFormula != "" {
		pq, err := skuresolve.ApplyFormula(skuresolve.JMESPath(sku.PricingFormula), mg)
		suite.Require().NoError(err, skuName)
		pricing.Quantity = pq
	}
	if usage.Unit != pricing.Unit {
		unit := suite.units[unitsPair{usage.Unit, pricing.Unit}]
		suite.Require().NotZero(unit.Factor, skuName)
		pricing.Quantity = pricing.Quantity.Div(unit.Factor)
	}
	return pricing
}
