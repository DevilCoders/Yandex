package origin

import (
	"fmt"
	"testing"
	"time"

	"github.com/stretchr/testify/suite"

	"a.yandex-team.ru/cloud/billing/go/pkg/decimal"
)

type bundleSkuTestSuite struct {
	suite.Suite
	value BundleSkus
}

func TestBundleSKU(t *testing.T) {
	suite.Run(t, new(bundleSkuTestSuite))
}

func (suite *bundleSkuTestSuite) SetupTest() {
	suite.initData()
}

func (suite *bundleSkuTestSuite) initData() {
	suite.value = BundleSkus{
		"valid.sku": {
			ID: "00000000000000000",
			Prices: []SkuPrice{
				{
					StartDate: YamlTime(time.Now()),
					Price:     decimal.Must(decimal.FromInt64(42)),
				},
			},
		},
	}
}

func (suite *bundleSkuTestSuite) getTestSku() BundleSku {
	suite.initData()
	return suite.value["valid.sku"]
}

func (suite *bundleSkuTestSuite) validateSku(sku BundleSku) error {
	suite.value["valid.sku"] = sku
	return suite.value.Valid()
}

func (suite *bundleSkuTestSuite) TestValid() {
	err := suite.value.Valid()
	suite.NoError(err)
}

func (suite *bundleSkuTestSuite) TestInvalidName() {
	cases := []string{
		"", "Invalid",
	}
	for _, c := range cases {
		suite.Run(fmt.Sprintf("case-%s", c), func() {
			suite.initData()
			suite.value[c] = suite.value["valid.sku"]
			err := suite.value.Valid()
			suite.Error(err)
		})
	}
}

func (suite *bundleSkuTestSuite) TestInvalidID() {
	cases := []string{
		"", "000", "A0000000000000000", "z0000000000000000",
	}
	for _, c := range cases {
		suite.Run(fmt.Sprintf("case-%s", c), func() {
			checkVal := suite.getTestSku()
			checkVal.ID = c
			suite.Error(suite.validateSku(checkVal))
		})
	}
}

func (suite *bundleSkuTestSuite) TestPrice() {
	{
		suite.initData()
		checkVal := suite.getTestSku()
		checkVal.Prices[0].StartDate = YamlTime{}
		suite.Error(suite.validateSku(checkVal))
	}
	{
		suite.initData()
		checkVal := suite.getTestSku()
		checkVal.Prices[0].StartDate = YamlTime(time.Now().Add(time.Hour))
		suite.Error(suite.validateSku(checkVal))
	}
	{
		suite.initData()
		checkVal := suite.getTestSku()
		checkVal.Prices[0].Price = decimal.Must(decimal.FromInt64(-42))
		suite.Error(suite.validateSku(checkVal))
	}
	{
		suite.initData()
		checkVal := suite.getTestSku()
		checkVal.Prices[0].Rates = append(checkVal.Prices[0].Rates, PriceRate{})
		suite.Error(suite.validateSku(checkVal))
	}
	{ // invalid dates order
		suite.initData()
		checkVal := suite.getTestSku()
		checkVal.Prices = append(checkVal.Prices, checkVal.Prices[0])
		suite.Error(suite.validateSku(checkVal))
	}
}

func (suite *bundleSkuTestSuite) TestInvalidRate() {
	dec := decimal.Must(decimal.FromInt64(42))
	cases := []PriceRate{
		{Quantity: dec.Neg()},
		{Quantity: dec, Price: dec.Neg()},
	}
	for i, c := range cases {
		suite.Run(fmt.Sprintf("case-%d", i), func() {
			suite.initData()
			checkVal := suite.getTestSku()
			checkVal.Prices[0].Price = decimal.Decimal128{}
			checkVal.Prices[0].Rates = []PriceRate{{}, c}
			suite.Error(suite.validateSku(checkVal))
		})
	}
}

func (suite *bundleSkuTestSuite) TestInvalidRatesOrder() {
	dec := decimal.Must(decimal.FromInt64(42))
	bigDec := decimal.Must(decimal.FromInt64(99))
	{
		suite.initData()
		checkVal := suite.getTestSku()
		checkVal.Prices[0].Price = decimal.Decimal128{}
		checkVal.Prices[0].Rates = []PriceRate{
			{Quantity: decimal.Decimal128{}},
			{Quantity: dec},
			{Quantity: bigDec},
		}
		suite.NoError(suite.validateSku(checkVal))
	}
	{
		suite.initData()
		checkVal := suite.getTestSku()
		checkVal.Prices[0].Price = decimal.Decimal128{}
		checkVal.Prices[0].Rates = []PriceRate{{Quantity: dec}}
		suite.Error(suite.validateSku(checkVal))
	}
	{
		suite.initData()
		checkVal := suite.getTestSku()
		checkVal.Prices[0].Price = decimal.Decimal128{}
		checkVal.Prices[0].Rates = []PriceRate{
			{Quantity: decimal.Decimal128{}},
			{Quantity: bigDec},
			{Quantity: dec},
		}
		suite.Error(suite.validateSku(checkVal))
	}
}

type skuTestSuite struct {
	suite.Suite
	value ServiceSkus
}

func TestSKU(t *testing.T) {
	suite.Run(t, new(skuTestSuite))
}

func (suite *skuTestSuite) SetupTest() {
	suite.initData()
}

func (suite *skuTestSuite) initData() {
	suite.value = ServiceSkus{
		Service: "valid.service-name_42",
		Skus: map[string]Sku{
			"valid.sku": {
				SkuNames: SkuNames{
					Ru: "Тестовый SKU",
					En: "Test SKU",
				},
				SkuAnalytics: SkuAnalytics{
					ReportingService: "valid.service-name_42/sub_service",
					Private:          true,
				},
				SkuUsage: SkuUsage{
					PricingFormula: "usage.quantity",
					UsageType:      DeltaUsage,
					Units: SkuUnits{
						Usage:   "usage-unit",
						Pricing: "pricing*unit",
					},
				},
				SkuResolving: SkuResolving{
					Schemas: []string{"without-tags", "with-tags"},
				},
			},
		},
	}
}

func (suite *skuTestSuite) getTestSku() Sku {
	suite.initData()
	return suite.value.Skus["valid.sku"]
}

func (suite *skuTestSuite) validateSku(sku Sku) error {
	suite.value.Skus["valid.sku"] = sku
	return suite.value.Valid()
}

func (suite *skuTestSuite) TestValid() {
	err := suite.value.Valid()
	suite.NoError(err)
}

func (suite *skuTestSuite) TestServiceName() {
	cases := []string{
		"", "Invalid",
	}
	for _, c := range cases {
		suite.Run(fmt.Sprintf("case-%s", c), func() {
			suite.initData()
			suite.value.Service = c
			err := suite.value.Valid()
			suite.Error(err)
		})
	}
}

func (suite *skuTestSuite) TestSkus() {
	{
		suite.initData()
		suite.value.Skus = nil
		err := suite.value.Valid()
		suite.Error(err)
	}
	{
		suite.initData()
		suite.value.Skus[""] = suite.value.Skus["valid.sku"]
		err := suite.value.Valid()
		suite.Error(err)
	}
	{
		suite.initData()
		suite.value.Skus["Invalid"] = suite.value.Skus["valid.sku"]
		err := suite.value.Valid()
		suite.Error(err)
	}
	{
		suite.initData()
		suite.value.Skus["valid.sku"] = Sku{}
		err := suite.value.Valid()
		suite.Error(err)
	}
}

func (suite *skuTestSuite) TestNames() {
	{
		checkVal := suite.getTestSku()
		checkVal.En = ""
		suite.Error(suite.validateSku(checkVal))
	}
	{
		checkVal := suite.getTestSku()
		checkVal.Ru = ""
		suite.Error(suite.validateSku(checkVal))
	}
}

func (suite *skuTestSuite) TestAnalytics() {
	cases := []string{
		"", "/test", "service/subservice/else",
	}
	for _, c := range cases {
		suite.Run(fmt.Sprintf("case-%s", c), func() {
			checkVal := suite.getTestSku()
			checkVal.ReportingService = c
			suite.Error(suite.validateSku(checkVal))
		})
	}
}

func (suite *skuTestSuite) TestUsage() {
	suite.Run("invalid formula", func() {
		checkVal := suite.getTestSku()
		checkVal.PricingFormula = "somethind strange"
		suite.Error(suite.validateSku(checkVal))
	})

	cases := []string{
		"",
	}
	for _, c := range cases {
		suite.Run(fmt.Sprintf("pricing_unit=%s", c), func() {
			checkVal := suite.getTestSku()
			checkVal.Units.Pricing = c
			suite.Error(suite.validateSku(checkVal))
		})
		suite.Run(fmt.Sprintf("usage_unit=%s", c), func() {
			checkVal := suite.getTestSku()
			checkVal.Units.Usage = c
			suite.Error(suite.validateSku(checkVal))
		})
	}
}

func (suite *skuTestSuite) TestDefaultUsage() {
	{
		checkVal := suite.getTestSku()
		checkVal.PricingFormula = "" // use default value
		suite.NoError(suite.validateSku(checkVal))
	}
}

func (suite *skuTestSuite) TestResolvingSchemas() {
	{
		checkVal := suite.getTestSku()
		checkVal.Schemas = nil
		suite.Error(suite.validateSku(checkVal))
	}
	{
		checkVal := suite.getTestSku()
		checkVal.Schemas = append(checkVal.Schemas, checkVal.Schemas[0])
		suite.Error(suite.validateSku(checkVal))
	}
	{
		checkVal := suite.getTestSku()
		checkVal.Schemas = append(checkVal.Schemas, "InvalidValue")
		suite.Error(suite.validateSku(checkVal))
	}
	{
		checkVal := suite.getTestSku()
		checkVal.Schemas = append(checkVal.Schemas, "")
		suite.Error(suite.validateSku(checkVal))
	}
}

func (suite *skuTestSuite) TestResolvingPolicy() {
	{
		checkVal := suite.getTestSku()
		checkVal.ResolvingPolicy = "usage.quantity"
		suite.NoError(suite.validateSku(checkVal))
	}
	{
		checkVal := suite.getTestSku()
		checkVal.ResolvingPolicy = "somethind strange"
		suite.Error(suite.validateSku(checkVal))
	}
}

func (suite *skuTestSuite) TestResolvingRules() {
	{
		checkVal := suite.getTestSku()
		checkVal.ResolvingRules = []map[string]SkuResolveRuleOption{{
			"tag": {Null: true},
		}}
		suite.NoError(suite.validateSku(checkVal))
	}
	{
		checkVal := suite.getTestSku()
		checkVal.ResolvingPolicy = "`true`"
		checkVal.ResolvingRules = []map[string]SkuResolveRuleOption{{
			"tag": {Null: true},
		}}
		suite.Error(suite.validateSku(checkVal))
	}
	{
		checkVal := suite.getTestSku()
		checkVal.ResolvingRules = []map[string]SkuResolveRuleOption{{
			"tag": {Null: true, Str: "smth"},
		}}
		suite.Error(suite.validateSku(checkVal))
	}
	{
		checkVal := suite.getTestSku()
		checkVal.ResolvingRules = []map[string]SkuResolveRuleOption{{}}
		suite.Error(suite.validateSku(checkVal))
	}
	{
		checkVal := suite.getTestSku()
		checkVal.ResolvingRules = []map[string]SkuResolveRuleOption{{
			"": {},
		}}
		suite.Error(suite.validateSku(checkVal))
	}
}
