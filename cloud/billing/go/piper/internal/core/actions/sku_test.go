package actions

import (
	"context"
	"fmt"
	"testing"
	"time"

	"github.com/stretchr/testify/mock"
	"github.com/stretchr/testify/suite"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/core/actions/mocks"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/core/entities"
	"a.yandex-team.ru/cloud/billing/go/pkg/decimal"
)

type skuTestSuite struct {
	suite.Suite
	mapper               mocks.SkuMapper
	converter            mocks.UnitsConverter
	cumulativeCalculator mocks.CumulativeCalculator

	sku             entities.Sku
	skuResolveRules entities.SkuResolveRules
	enrichedMetric  entities.EnrichedMetric
}

func TestSkuResolving(t *testing.T) {
	suite.Run(t, new(skuTestSuite))
}

func (suite *skuTestSuite) SetupTest() {
	suite.mapper = mocks.SkuMapper{}
	suite.converter = mocks.UnitsConverter{}
	suite.cumulativeCalculator = mocks.CumulativeCalculator{}

	suite.sku = entities.Sku{
		SkuID:   "compute_sku",
		SkuInfo: entities.SkuInfo{},
		SkuUsageInfo: entities.SkuUsageInfo{
			Formula: "usage.quantity",
		},
	}
	suite.skuResolveRules = entities.SkuResolveRules{
		Key: "compute",
		Rules: []entities.SkuResolve{
			{
				SkuID: "compute_sku",
				Rules: []entities.MatchRulesByPath{{
					"tags.notexists": {Type: entities.ExistenceRule, Exists: false},
				}},
			},
		},
	}

	suite.enrichedMetric = entities.EnrichedMetric{}
	suite.enrichedMetric.Schema = "compute"
	suite.enrichedMetric.Tags = `{"cores": 3, "bad": {}}`
	suite.enrichedMetric.Usage.Quantity = toDec(10)
}

func toDec(v int64) decimal.Decimal128 {
	return decimal.Must(decimal.FromInt64(v))
}

func (suite *skuTestSuite) TestProratedBlacklisted() {
	cases := []struct {
		skuID string
		want  bool
	}{
		{"dn23ggtbq6i06bq934j7", true},
		{"dn2ni2kmom41tf568dql", true},
		{"ni2kmom41tf568dql", false},
		{"dn2somethingelse", false},
	}

	for _, c := range cases {
		suite.Run(c.skuID, func() {
			suite.EqualValues(proratedBlacklisted(c.skuID), c.want)
		})
	}
}

func (suite *skuTestSuite) TestProratedBlacklistedAllBlacklistedSku() {
	for skuID := range proratedSkuBlacklist {
		suite.True(proratedBlacklisted(fmt.Sprintf("dn2%s", skuID)))
	}
}

func (suite *skuTestSuite) TestResolveMetricsSku() {
	metric := suite.enrichedMetric
	metrics := []entities.EnrichedMetric{metric}

	suite.mapper.On("GetSkuResolving", mock.Anything, mock.Anything, []string{"compute"}, mock.Anything, mock.Anything).
		Return([]entities.SkuResolveRules{suite.skuResolveRules}, nil, []entities.Sku{suite.sku}, nil)
	suite.converter.On("ConvertQuantity", mock.Anything, mock.Anything, mock.Anything, "", "", metric.Usage.Quantity).
		Return(metric.Usage.Quantity, nil)

	valid, invalid, err := ResolveMetricsSku(context.Background(), entities.ProcessingScope{}, &suite.mapper, &suite.converter, metrics)

	suite.Require().NoError(err)
	suite.Empty(invalid)
	suite.Equal("compute_sku", valid[0].SkuID)
	suite.Equal(metric.Usage.Quantity, valid[0].PricingQuantity)

	suite.mapper.AssertExpectations(suite.T())
	suite.converter.AssertExpectations(suite.T())
}

func (suite *skuTestSuite) TestResolveMetricsExplicitSku() {
	expSku := suite.sku
	expSku.SkuID = "explicit"

	metric := suite.enrichedMetric
	metric.SkuID = expSku.SkuID
	metrics := []entities.EnrichedMetric{metric}

	suite.mapper.On("GetSkuResolving", mock.Anything, mock.Anything, []string{"compute"}, mock.Anything, []string{metric.SkuID}).
		Return([]entities.SkuResolveRules{suite.skuResolveRules}, nil, []entities.Sku{suite.sku, expSku}, nil)
	suite.converter.On("ConvertQuantity", mock.Anything, mock.Anything, mock.Anything, "", "", metric.Usage.Quantity).
		Return(metric.Usage.Quantity, nil)

	valid, invalid, err := ResolveMetricsSku(context.Background(), entities.ProcessingScope{}, &suite.mapper, &suite.converter, metrics)

	suite.Require().NoError(err)
	suite.Empty(invalid)
	suite.Equal("explicit", valid[0].SkuID)
	suite.Equal(metric.Usage.Quantity, valid[0].PricingQuantity)

	suite.mapper.AssertExpectations(suite.T())
	suite.converter.AssertExpectations(suite.T())
}

func (suite *skuTestSuite) TestResolveMetricsSkuComputeError() {
	metric := suite.enrichedMetric
	metrics := []entities.EnrichedMetric{metric}
	suite.sku.Formula = "usage.quantity * tags.bad"

	suite.mapper.On("GetSkuResolving", mock.Anything, mock.Anything, []string{"compute"}, mock.Anything, mock.Anything).
		Return([]entities.SkuResolveRules{suite.skuResolveRules}, nil, []entities.Sku{suite.sku}, nil)

	valid, invalid, err := ResolveMetricsSku(context.Background(), entities.ProcessingScope{}, &suite.mapper, &suite.converter, metrics)

	suite.Require().NoError(err)
	suite.Empty(valid)
	suite.Len(invalid, 1)

	suite.mapper.AssertExpectations(suite.T())
	suite.converter.AssertExpectations(suite.T())
}

func (suite *skuTestSuite) TestResolveMetricsSkuWithProducts() {
	metric := suite.enrichedMetric
	metric.Schema = "compute.vm.generic.v1"
	metric.Products = []string{"compute.vm.generic.v1"}
	metrics := []entities.EnrichedMetric{metric}

	skuResolveRules := suite.skuResolveRules
	skuResolveRules.Key = "compute.vm.generic.v1"

	suite.mapper.On("GetSkuResolving", mock.Anything, mock.Anything, mock.Anything, []string{"compute.vm.generic.v1"}, mock.Anything).
		Return(nil, []entities.SkuResolveRules{skuResolveRules}, []entities.Sku{suite.sku}, nil)
	suite.converter.On("ConvertQuantity", mock.Anything, mock.Anything, mock.Anything, "", "", metric.Usage.Quantity).
		Return(metric.Usage.Quantity, nil)

	valid, invalid, err := ResolveMetricsSku(context.Background(), entities.ProcessingScope{}, &suite.mapper, &suite.converter, metrics)

	suite.Require().NoError(err)
	suite.Empty(invalid)
	suite.Equal("compute_sku", valid[0].SkuID)
	suite.Equal(metric.Usage.Quantity, valid[0].PricingQuantity)

	suite.mapper.AssertExpectations(suite.T())
	suite.converter.AssertExpectations(suite.T())
}

func (suite *skuTestSuite) TestResolveMetricsSkuWithProductsComputeError() {
	metric := suite.enrichedMetric
	metric.Schema = "compute.vm.generic.v1"
	metric.Products = []string{"compute.vm.generic.v1"}
	metrics := []entities.EnrichedMetric{metric}

	skuResolveRules := suite.skuResolveRules
	skuResolveRules.Key = "compute.vm.generic.v1"
	suite.sku.Formula = "usage.quantity * tags.bad"

	suite.mapper.On("GetSkuResolving", mock.Anything, mock.Anything, mock.Anything, []string{"compute.vm.generic.v1"}, mock.Anything).
		Return(nil, []entities.SkuResolveRules{skuResolveRules}, []entities.Sku{suite.sku}, nil)

	valid, invalid, err := ResolveMetricsSku(context.Background(), entities.ProcessingScope{}, &suite.mapper, &suite.converter, metrics)

	suite.Require().NoError(err)
	suite.Empty(valid)
	suite.Len(invalid, 1)

	suite.mapper.AssertExpectations(suite.T())
	suite.converter.AssertExpectations(suite.T())
}

func (suite *skuTestSuite) TestCountCumulativeProratedMonthlyUsage() {
	cases := []struct {
		skuID       string
		blacklisted bool
	}{
		{"dn23ggtbq6i06bq934j7", true},
		{"dn2somethingelse", false},
	}

	for _, c := range cases {
		suite.Run(c.skuID, func() {
			quantity1 := toDec(10)
			quantity2 := toDec(20)
			quantity3 := toDec(5)
			quantities := []decimal.Decimal128{quantity1, quantity2, quantity3}
			var metrics []entities.EnrichedMetric

			for idx, quantity := range quantities {
				metric := entities.EnrichedMetric{}
				metric.SkuUsageType = entities.CumulativeUsage
				metric.ResourceID = "resource_id"
				metric.SkuID = c.skuID
				metric.PricingQuantity = quantity
				if idx <= 1 {
					metric.Usage.Finish, _ = time.Parse(time.RFC3339, "2020-01-10T00:00:00+03:00")
				} else {
					metric.Usage.Finish, _ = time.Parse(time.RFC3339, "2020-02-15T12:00:00+03:00")
				}
				metrics = append(metrics, metric)
			}

			cumulativeSource1 := entities.CumulativeSource{
				ResourceID:      "resource_id",
				SkuID:           c.skuID,
				PricingQuantity: quantity2,
				MetricOffset:    0,
			}
			cumulativeSource2 := entities.CumulativeSource{
				ResourceID:      "resource_id",
				SkuID:           c.skuID,
				PricingQuantity: quantity3,
				MetricOffset:    0,
			}

			period1, _ := time.Parse(time.RFC3339, "2020-01-01T00:00:00+03:00")
			period2, _ := time.Parse(time.RFC3339, "2020-02-01T00:00:00+03:00")

			cumulativeUsage1 := entities.CumulativeUsageLog{
				FirstPeriod:  false,
				ResourceID:   "resource_id",
				SkuID:        c.skuID,
				MetricOffset: 0,
				Delta:        quantity2,
			}
			cumulativeUsage2 := entities.CumulativeUsageLog{
				FirstPeriod:  true,
				ResourceID:   "resource_id",
				SkuID:        c.skuID,
				MetricOffset: 0,
				Delta:        quantity3,
			}

			suite.cumulativeCalculator.On("CalculateCumulativeUsage", mock.Anything, mock.Anything,
				mock.MatchedBy(func(p entities.UsagePeriod) bool { return p.Period.Equal(period1) }),
				[]entities.CumulativeSource{cumulativeSource1}).
				Return(1, []entities.CumulativeUsageLog{cumulativeUsage1}, nil)

			suite.cumulativeCalculator.On("CalculateCumulativeUsage", mock.Anything, mock.Anything,
				mock.MatchedBy(func(p entities.UsagePeriod) bool { return p.Period.Equal(period2) }),
				[]entities.CumulativeSource{cumulativeSource2}).
				Return(1, []entities.CumulativeUsageLog{cumulativeUsage2}, nil)

			valid, err := CountCumulativeMonthlyUsageProrated(context.Background(), entities.ProcessingScope{}, &suite.cumulativeCalculator, metrics)

			suite.Require().NoError(err)
			suite.Require().Zero(valid[0].PricingQuantity)
			suite.Require().Equal(quantity2, valid[1].PricingQuantity)

			if !c.blacklisted {
				suite.Require().Positive(quantity3.Sub(valid[2].PricingQuantity).Sign())
			} else {
				suite.Require().Equal(quantity3, valid[2].PricingQuantity)
			}

			suite.cumulativeCalculator.AssertExpectations(suite.T())
		})
	}
}

func (suite *skuTestSuite) TestCountCumulativeMonthlyUsage() {
	quantity1 := toDec(10)
	quantity2 := toDec(20)
	quantity3 := toDec(5)
	quantities := []decimal.Decimal128{quantity1, quantity2, quantity3}
	var metrics []entities.EnrichedMetric

	for idx, quantity := range quantities {
		metric := entities.EnrichedMetric{}
		metric.SkuUsageType = entities.CumulativeUsage
		metric.ResourceID = "resource_id"
		metric.SkuID = "sku_id"
		metric.PricingQuantity = quantity
		if idx <= 1 {
			metric.Usage.Finish, _ = time.Parse(time.RFC3339, "2020-01-10T00:00:00+03:00")
		} else {
			metric.Usage.Finish, _ = time.Parse(time.RFC3339, "2020-02-10T00:00:00+03:00")
		}
		metrics = append(metrics, metric)
	}

	cumulativeSource1 := entities.CumulativeSource{
		ResourceID:      "resource_id",
		SkuID:           "sku_id",
		PricingQuantity: quantity2,
		MetricOffset:    0,
	}
	cumulativeSource2 := entities.CumulativeSource{
		ResourceID:      "resource_id",
		SkuID:           "sku_id",
		PricingQuantity: quantity3,
		MetricOffset:    0,
	}

	period1, _ := time.Parse(time.RFC3339, "2020-01-01T00:00:00+03:00")
	period2, _ := time.Parse(time.RFC3339, "2020-02-01T00:00:00+03:00")

	cumulativeUsage1 := entities.CumulativeUsageLog{
		FirstPeriod:  false,
		ResourceID:   "resource_id",
		SkuID:        "sku_id",
		MetricOffset: 0,
		Delta:        quantity2,
	}
	cumulativeUsage2 := entities.CumulativeUsageLog{
		FirstPeriod:  false,
		ResourceID:   "resource_id",
		SkuID:        "sku_id",
		MetricOffset: 0,
		Delta:        quantity3,
	}

	suite.cumulativeCalculator.On("CalculateCumulativeUsage", mock.Anything, mock.Anything,
		mock.MatchedBy(func(p entities.UsagePeriod) bool { return p.Period.Equal(period1) }),
		[]entities.CumulativeSource{cumulativeSource1}).
		Return(1, []entities.CumulativeUsageLog{cumulativeUsage1}, nil)

	suite.cumulativeCalculator.On("CalculateCumulativeUsage", mock.Anything, mock.Anything,
		mock.MatchedBy(func(p entities.UsagePeriod) bool { return p.Period.Equal(period2) }),
		[]entities.CumulativeSource{cumulativeSource2}).
		Return(1, []entities.CumulativeUsageLog{cumulativeUsage2}, nil)

	valid, err := CountCumulativeMonthlyUsage(context.Background(), entities.ProcessingScope{}, &suite.cumulativeCalculator, metrics)

	suite.Require().NoError(err)
	suite.Require().Zero(valid[0].PricingQuantity)
	suite.Require().Equal(quantity2, valid[1].PricingQuantity)
	suite.Require().Equal(quantity3, valid[2].PricingQuantity)

	suite.cumulativeCalculator.AssertExpectations(suite.T())
}

func (suite *skuTestSuite) TestComputeE2EResolveIssue() {
	metric := suite.enrichedMetric
	metric.Schema = "compute.vm.generic.v1"
	metric.Tags = `{
		"platform_id": "e2e",
		"memory": 1073741824,
		"cores": 2,
		"compute_node_system_cores": 20,
		"compute_node_total_cores": 56,
		"nvme_disks": 0,
		"sockets": 1,
		"product_ids": [],
		"gpus": 0,
		"core_fraction": 5,
		"public_fips": 0
	}`

	metrics := []entities.EnrichedMetric{metric}

	skuResolveRules := entities.SkuResolveRules{
		Key: "compute.vm.generic.v1",
		Rules: []entities.SkuResolve{
			{
				SkuID: "compute_sku",
				Rules: []entities.MatchRulesByPath{
					{
						"tags.core_fraction": {Type: entities.ValueRule, Value: entities.ValueMatch{Number: decimal.Must(decimal.FromInt64(5))}},
						"tags.preemptible":   {Type: entities.ValueRule, Value: entities.ValueMatch{Number: decimal.Must(decimal.FromInt64(0))}},
						"tags.platform_id":   {Type: entities.ValueRule, Value: entities.ValueMatch{String: "e2e"}},
					},
					{
						"tags.core_fraction": {Type: entities.ValueRule, Value: entities.ValueMatch{Number: decimal.Must(decimal.FromInt64(5))}},
						"tags.preemptible":   {Type: entities.ValueRule, Value: entities.ValueMatch{Null: true}},
						"tags.platform_id":   {Type: entities.ValueRule, Value: entities.ValueMatch{String: "e2e"}},
					},
				},
			},
		},
	}

	suite.mapper.On("GetSkuResolving", mock.Anything, mock.Anything, mock.Anything, []string{"compute.vm.generic.v1"}, mock.Anything).
		Return([]entities.SkuResolveRules{skuResolveRules}, nil, []entities.Sku{suite.sku}, nil)
	suite.converter.On("ConvertQuantity", mock.Anything, mock.Anything, mock.Anything, "", "", metric.Usage.Quantity).
		Return(metric.Usage.Quantity, nil)

	valid, invalid, err := ResolveMetricsSku(context.Background(), entities.ProcessingScope{}, &suite.mapper, &suite.converter, metrics)

	suite.Require().NoError(err)
	suite.Empty(invalid)
	suite.Equal("compute_sku", valid[0].SkuID)
	suite.Equal(metric.Usage.Quantity, valid[0].PricingQuantity)

	suite.mapper.AssertExpectations(suite.T())
	suite.converter.AssertExpectations(suite.T())
}
