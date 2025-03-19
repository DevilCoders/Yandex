package ydb

import (
	"testing"

	"github.com/stretchr/testify/suite"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/core/entities"
	"a.yandex-team.ru/cloud/billing/go/pkg/decimal"
)

type skuTestSuite struct {
	baseMetaSessionTestSuite
}

func TestSku(t *testing.T) {
	suite.Run(t, new(skuTestSuite))
}

func (suite *skuTestSuite) TestGetResolving() {
	const rules = `[
		{
			"options":{"tags.storage_class":{"value":"STANDARD"},"tags.method":{"value":"OPTIONS"}},
			"hashable_values":["OPTIONS","STANDARD"],
			"options_hash":14412897823742927000
		},{
			"options":{"tags.storage_class":{"value":"UNKNOWN"},"tags.method":{"value":"OPTIONS"}},
			"hashable_values":["OPTIONS","UNKNOWN"],
			"options_hash":14412897823742927000
		}
	]`
	skuResult := suite.mock.NewRows([]string{"id", "name", "pricing_unit", "formula", "usage_unit", "usage_type"}).
		AddRow("sku1", "SKU_1", "pu", "usage.quantity", "uu", "delta").
		AddRow("sku2", "SKU_2", "pu", "`1`", "uu", "cumulative").
		AddRow("sku3", "SKU_3", "pu", "`true`", "uu", "gauge").
		AddRow("explicit", "SKU_exp", "pu", "`true`", "uu", "delta").
		AddRow("bad", "incorrect", "pu", "`false`", "uu", "error")
	schemasResult := suite.mock.NewRows([]string{"id", "schema", "resolving_rules", "resolving_policy"}).
		AddRow("sku1", "scm1", rules, nil).
		AddRow("sku2", "scm2", nil, "`true`").
		AddRow("deprecated", "scm3", nil, "`false`")
	productsResult := suite.mock.NewRows([]string{"id", "product_id", "resolving_rules", "check_formula"}).
		AddRow("sku2", "prd1", rules, nil).
		AddRow("sku3", "prd2", nil, "`true`").
		AddRow("deprecated", "prd3", nil, "`false`")
	suite.mock.ExpectBegin()
	suite.mock.ExpectQuery("FROM `meta/skus`").
		WillReturnRows(schemasResult, productsResult, skuResult).
		RowsWillBeClosed()
	suite.mock.ExpectCommit()

	schemaToSku, productToSku, skus, err := suite.session.GetSkuResolving(suite.ctx, entities.ProcessingScope{},
		[]string{"scm1", "scm2", "scm3"}, []string{"prd1", "prd2", "prd3"}, []string{"explicit"},
	)
	suite.Require().NoError(err)

	wantSku := []entities.Sku{
		{
			SkuID:        "sku1",
			SkuInfo:      entities.SkuInfo{SkuName: "SKU_1", PricingUnit: "pu", SkuUsageType: entities.DeltaUsage},
			SkuUsageInfo: entities.SkuUsageInfo{Formula: "usage.quantity", UsageUnit: "uu"},
		},
		{
			SkuID:        "sku2",
			SkuInfo:      entities.SkuInfo{SkuName: "SKU_2", PricingUnit: "pu", SkuUsageType: entities.CumulativeUsage},
			SkuUsageInfo: entities.SkuUsageInfo{Formula: "`1`", UsageUnit: "uu"},
		},
		{
			SkuID:        "sku3",
			SkuInfo:      entities.SkuInfo{SkuName: "SKU_3", PricingUnit: "pu", SkuUsageType: entities.GaugeUsage},
			SkuUsageInfo: entities.SkuUsageInfo{Formula: "`true`", UsageUnit: "uu"},
		},
		{
			SkuID:        "explicit",
			SkuInfo:      entities.SkuInfo{SkuName: "SKU_exp", PricingUnit: "pu", SkuUsageType: entities.DeltaUsage},
			SkuUsageInfo: entities.SkuUsageInfo{Formula: "`true`", UsageUnit: "uu"},
		},
		{
			SkuID:        "bad",
			SkuInfo:      entities.SkuInfo{SkuName: "incorrect", PricingUnit: "pu", SkuUsageType: entities.UnknownUsage},
			SkuUsageInfo: entities.SkuUsageInfo{Formula: "`false`", UsageUnit: "uu"},
		},
	}
	suite.ElementsMatch(wantSku, skus)

	wantSch := []entities.SkuResolveRules{
		{
			Key: "scm1",
			Rules: []entities.SkuResolve{
				{SkuID: "sku1", Rules: []entities.MatchRulesByPath{
					{
						"tags.storage_class": entities.MatchRule{
							Type:  entities.ValueRule,
							Value: entities.ValueMatch{String: "STANDARD"},
						},
						"tags.method": entities.MatchRule{
							Type:  entities.ValueRule,
							Value: entities.ValueMatch{String: "OPTIONS"},
						},
					},
					{
						"tags.storage_class": entities.MatchRule{
							Type:  entities.ValueRule,
							Value: entities.ValueMatch{String: "UNKNOWN"},
						},
						"tags.method": entities.MatchRule{
							Type:  entities.ValueRule,
							Value: entities.ValueMatch{String: "OPTIONS"},
						},
					},
				}},
			},
		},
		{
			Key: "scm2",
			Rules: []entities.SkuResolve{
				{SkuID: "sku2", Policy: "`true`"},
			},
		},
	}
	suite.ElementsMatch(wantSch, schemaToSku)
	wantPrd := []entities.SkuResolveRules{
		{
			Key: "prd1",
			Rules: []entities.SkuResolve{
				{SkuID: "sku2", Rules: []entities.MatchRulesByPath{
					{
						"tags.storage_class": entities.MatchRule{
							Type:  entities.ValueRule,
							Value: entities.ValueMatch{String: "STANDARD"},
						},
						"tags.method": entities.MatchRule{
							Type:  entities.ValueRule,
							Value: entities.ValueMatch{String: "OPTIONS"},
						},
					},
					{
						"tags.storage_class": entities.MatchRule{
							Type:  entities.ValueRule,
							Value: entities.ValueMatch{String: "UNKNOWN"},
						},
						"tags.method": entities.MatchRule{
							Type:  entities.ValueRule,
							Value: entities.ValueMatch{String: "OPTIONS"},
						},
					},
				}},
			},
		},
		{
			Key: "prd2",
			Rules: []entities.SkuResolve{
				{SkuID: "sku3", Policy: "`true`"},
			},
		},
	}
	suite.ElementsMatch(wantPrd, productToSku)
}

func (suite *skuTestSuite) TestGetResolvingDbErr() {
	suite.mock.ExpectBegin().WillReturnError(errTest)
	_, _, _, err := suite.session.GetSkuResolving(suite.ctx, entities.ProcessingScope{},
		[]string{"scm1", "scm2", "scm3"}, []string{"prd1", "prd2", "prd3"}, []string{"explicit"},
	)
	suite.Require().Error(err)
	suite.ErrorIs(err, errTest)
}

func (suite *skuTestSuite) TestGetResolvingIncorrectRules() {
	skuResult := suite.mock.NewRows([]string{"id", "name", "pricing_unit", "formula", "usage_unit", "usage_type"}).
		AddRow("sku1", "", "", "", "", "")
	schemasResult := suite.mock.NewRows([]string{"id", "schema", "resolving_rules", "resolving_policy"}).
		AddRow("sku1", "scm1", `"error"`, nil)
	productsResult := suite.mock.NewRows([]string{"id", "product_id", "resolving_rules", "check_formula"})
	suite.mock.ExpectBegin()
	suite.mock.ExpectQuery("FROM `meta/skus`").
		WillReturnRows(schemasResult, productsResult, skuResult).
		RowsWillBeClosed()
	suite.mock.ExpectCommit()

	sch, _, _, err := suite.session.GetSkuResolving(suite.ctx, entities.ProcessingScope{},
		[]string{"scm1", "scm2", "scm3"}, []string{"prd1", "prd2", "prd3"}, []string{"explicit"},
	)
	suite.Require().NoError(err)
	suite.Require().Len(sch, 1)
	suite.Require().Empty(sch[0].Rules[0].Rules)
}

func (suite *skuTestSuite) TestGetResolvingSchemaRulesError() {
	skuResult := suite.mock.NewRows([]string{"id", "name", "pricing_unit", "formula", "usage_unit", "usage_type"}).
		AddRow("sku1", "", "", "", "", "")
	schemasResult := suite.mock.NewRows([]string{"id", "schema", "resolving_rules", "resolving_policy"})
	productsResult := suite.mock.NewRows([]string{"id", "product_id", "resolving_rules", "check_formula"}).
		AddRow("sku1", "prd1", `"`, nil)
	suite.mock.ExpectBegin()
	suite.mock.ExpectQuery("FROM `meta/skus`").
		WillReturnRows(schemasResult, productsResult, skuResult).
		RowsWillBeClosed()
	suite.mock.ExpectCommit()

	_, _, _, err := suite.session.GetSkuResolving(suite.ctx, entities.ProcessingScope{},
		[]string{"scm1", "scm2", "scm3"}, []string{"prd1", "prd2", "prd3"}, []string{"explicit"},
	)
	suite.Require().Error(err)
	suite.ErrorIs(err, ErrResolveRulesValue)
}

func (suite *skuTestSuite) TestGetResolvingProductRulesError() {
	skuResult := suite.mock.NewRows([]string{"id", "name", "pricing_unit", "formula", "usage_unit", "usage_type"}).
		AddRow("sku1", "", "", "", "", "")
	schemasResult := suite.mock.NewRows([]string{"id", "schema", "resolving_rules", "resolving_policy"}).
		AddRow("sku1", "scm1", `"`, nil)
	productsResult := suite.mock.NewRows([]string{"id", "product_id", "resolving_rules", "check_formula"})
	suite.mock.ExpectBegin()
	suite.mock.ExpectQuery("FROM `meta/skus`").
		WillReturnRows(schemasResult, productsResult, skuResult).
		RowsWillBeClosed()
	suite.mock.ExpectCommit()

	_, _, _, err := suite.session.GetSkuResolving(suite.ctx, entities.ProcessingScope{},
		[]string{"scm1", "scm2", "scm3"}, []string{"prd1", "prd2", "prd3"}, []string{"explicit"},
	)
	suite.Require().Error(err)
	suite.ErrorIs(err, ErrResolveRulesValue)
}

func (suite *skuTestSuite) TestGetResolvingRules() {
	const rules = `[{"options":{
		"exst":{"exists":true},
		"not_exst":{"exists":false},
		"null":{"value":null},
		"empty_str":{"value":""},
		"str":{"value":"str"},
		"str_num":{"value":"1"},
		"true":{"value":true},
		"false":{"value":false},
		"num":{"value": 1}
	}}]`
	skuResult := suite.mock.NewRows([]string{"id", "name", "pricing_unit", "formula", "usage_unit", "usage_type"}).
		AddRow("sku1", "", "", "", "", "")
	schemasResult := suite.mock.NewRows([]string{"id", "schema", "resolving_rules", "resolving_policy"}).
		AddRow("sku1", "scm1", rules, nil)
	productsResult := suite.mock.NewRows([]string{"id", "product_id", "resolving_rules", "check_formula"})
	suite.mock.ExpectBegin()
	suite.mock.ExpectQuery("FROM `meta/skus`").
		WillReturnRows(schemasResult, productsResult, skuResult).
		RowsWillBeClosed()
	suite.mock.ExpectCommit()

	sch, _, _, err := suite.session.GetSkuResolving(suite.ctx, entities.ProcessingScope{},
		[]string{"scm1", "scm2", "scm3"}, []string{"prd1", "prd2", "prd3"}, []string{"explicit"},
	)
	suite.Require().NoError(err)

	want := entities.MatchRulesByPath{
		"exst":      entities.MatchRule{Type: entities.ExistenceRule, Exists: true},
		"not_exst":  entities.MatchRule{Type: entities.ExistenceRule, Exists: false},
		"null":      entities.MatchRule{Type: entities.ValueRule, Value: entities.ValueMatch{Null: true}},
		"empty_str": entities.MatchRule{Type: entities.ValueRule, Value: entities.ValueMatch{EmptyString: true}},
		"str":       entities.MatchRule{Type: entities.ValueRule, Value: entities.ValueMatch{String: "str"}},
		"str_num":   entities.MatchRule{Type: entities.ValueRule, Value: entities.ValueMatch{Number: decimal.Must(decimal.FromInt64(1))}},
		"true":      entities.MatchRule{Type: entities.ValueRule, Value: entities.ValueMatch{True: true}},
		"false":     entities.MatchRule{Type: entities.ValueRule, Value: entities.ValueMatch{False: true}},
		"num":       entities.MatchRule{Type: entities.ValueRule, Value: entities.ValueMatch{Number: decimal.Must(decimal.FromInt64(1))}},
	}
	got := sch[0].Rules[0].Rules[0]
	suite.EqualValues(want, got)
}
