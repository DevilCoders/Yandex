package applicables

import "a.yandex-team.ru/cloud/billing/go/pkg/decimal"

// Unit stores information about billing units
type Unit struct {
	SrcUnit string             `yaml:"src_unit"`
	DstUnit string             `yaml:"dst_unit"`
	Factor  decimal.Decimal128 `yaml:"factor"`
}

// Service stores information about billing service
type Service struct {
	ID          string `yaml:"id"`
	Name        string `yaml:"name"`
	Description string `yaml:"description"`
	Group       string `yaml:"group"`
}

// Schema stores information about billing services schemas
type Schema struct {
	Name      string      `yaml:"name"`
	ServiceID DummyString `yaml:"service_id"`
	Tags      SchemaTags  `yaml:"tags,omitempty"`
}

// SchemaTags contains constraints for billing metrics of specified schema
type SchemaTags struct {
	Required []string `yaml:"required,omitempty"`
	Optional []string `yaml:"optional,omitempty"`
}

// Sku is representation of billing stock keeping unit
type Sku struct {
	ID               string           `yaml:"id"`
	Name             string           `yaml:"name"`
	ServiceID        string           `yaml:"service_id"`
	Schemas          []string         `yaml:"schemas"`
	BalanceProductID DummyString      `yaml:"balance_product_id"`
	ResolvingPolicy  string           `yaml:"resolving_policy,omitempty"`
	ResolvingRules   []ResolvingRule  `yaml:"resolving_rules,omitempty"`
	Formula          string           `yaml:"formula"`
	UsageUnit        string           `yaml:"usage_unit"`
	PricingUnit      string           `yaml:"pricing_unit"`
	UsageType        string           `yaml:"usage_type"`
	Labels           SkuLabels        `yaml:"labels"`
	CreatedAt        int              `yaml:"created_at"`
	PricingVersions  []PricingVersion `yaml:"pricing_versions"`
	Translations     Translations     `yaml:"translations"`
}

// SkuLabels stores analytic labels (mostly DWH specific)
type SkuLabels struct {
	RealServiceID string `yaml:"real_service_id"`
	Subservice    string `yaml:"subservice"`
	Visibility    string `yaml:"visibility"`
}

// PricingVersion is price and time
type PricingVersion struct {
	EffectiveTime     int               `yaml:"effective_time"`
	PricingExpression PricingExpression `yaml:"pricing_expression"`
}

// PricingExpression is common pricing rates list with some legacy dummy info
type PricingExpression struct {
	Quantum QuantumConstant `yaml:"quantum"`
	Rates   []PricingRate   `yaml:"rates"`
}

// PricingRate stores thresholds and prices for SKU
type PricingRate struct {
	StartPricingQuantity decimal.Decimal128 `yaml:"start_pricing_quantity"`
	UnitPrice            decimal.Decimal128 `yaml:"unit_price"`
}

// Translations of sku name
type Translations struct {
	Ru TranslationName `yaml:"ru"`
	En TranslationName `yaml:"en"`
}

// TranslationName is dummy structure type
type TranslationName struct {
	Name string `yaml:"name"`
}

// ResolvingRule - great and complex type for storing matching rules very optimized in python
type ResolvingRule map[string]ResolvingRuleOption
