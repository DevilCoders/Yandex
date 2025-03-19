package origin

import (
	"errors"
	"fmt"
	"time"

	"a.yandex-team.ru/cloud/billing/go/pkg/decimal"
	"a.yandex-team.ru/cloud/billing/go/pkg/selfsku/tools"
	"a.yandex-team.ru/library/go/valid"
)

// BundleSkus is final set of SKUs to build one bundle
type BundleSkus map[string]BundleSku

func (b BundleSkus) Valid() error {
	errs := tools.ValidErrors{}
	wrap := struct {
		BundleSkus `valid:"keys=name,each=valid"`
	}{b}

	errs.Collect(valid.Struct(vctx, wrap))
	return errs.Expose()
}

// BundleSku stores info specific for each bundle
type BundleSku struct {
	ID     string     `yaml:"id" valid:"cloudid"`
	Prices []SkuPrice `yaml:"prices" valid:"not-empty,each=valid"`
}

func (b BundleSku) Valid() error {
	errs := tools.ValidErrors{}
	var prevStart time.Time
	for i, p := range b.Prices {
		if i == 0 {
			if time.Time(p.StartDate).After(time.Now()) { // Ugly check from python
				errs.Collect(errors.New("first price should start before now"))
			}
			prevStart = time.Time(p.StartDate)
			continue
		}
		sd := time.Time(p.StartDate)
		if !sd.After(prevStart) {
			errs.Collect(fmt.Errorf("price %d(%s) should start after %s", i, sd.String(), prevStart.String()))
		}
		prevStart = sd
	}
	return errs.Expose()
}

// SkuPrice is one version of SKU price effective in some time range
type SkuPrice struct {
	StartDate YamlTime           `yaml:"start_date" valid:"not-empty"`
	Price     decimal.Decimal128 `yaml:"price" valid:"not-negative"`
	Rates     []PriceRate        `yaml:"rates"`
	// TODO: Aggregation. Now only BA+month
}

func (p SkuPrice) Valid() error {
	errs := tools.ValidErrors{}
	if !p.Price.IsZero() && len(p.Rates) > 0 {
		errs.Collect(errors.New("can not use price and rates simultaneously"))
	}
	var qty decimal.Decimal128
	for i, r := range p.Rates {
		if i == 0 {
			if !r.Quantity.IsZero() {
				errs.Collect(errors.New("first rate quantity should be 0"))
			}
			qty = r.Quantity
			continue
		}
		if r.Quantity.Cmp(qty) <= 0 {
			errs.Collect(fmt.Errorf("price rate quantity %d(%f) should be grater than %f", i, r.Quantity, qty))
		}
		qty = r.Quantity
	}
	return errs.Expose()
}

// PriceRate for complex prices with various thresholds
type PriceRate struct {
	Quantity decimal.Decimal128 `yaml:"quantity" valid:"not-negative"`
	Price    decimal.Decimal128 `yaml:"price" valid:"not-negative"`
}

// ServiceSkus stores group of skus with same service
type ServiceSkus struct {
	Service string         `yaml:"service" valid:"name"`
	Skus    map[string]Sku `yaml:"skus" valid:"not-empty,keys=name,each=valid"`
}

func (s ServiceSkus) Valid() error {
	return valid.Struct(vctx, s)
}

// Sku stores general information common to all bundles
type Sku struct {
	SkuNames     `yaml:",inline"`
	SkuAnalytics `yaml:",inline"`
	SkuUsage     `yaml:",inline"`
	SkuResolving `yaml:",inline"`
}

// SkuNames in various translations
type SkuNames struct {
	Ru string `yaml:"ru" valid:"not-empty"`
	En string `yaml:"en" valid:"not-empty"`
}

// SkuAnalytics is for generating SKU labels (DWH specific)
type SkuAnalytics struct {
	ReportingService string `yaml:"reporting_service" valid:"hierarchy=2"`
	Private          bool   `yaml:"private"`
}

// SkuUsage defines units of billing metric usage, final pricing unit and
// method (see cumulative case) and formula to convert usage to pricing
type SkuUsage struct {
	PricingFormula string    `yaml:"pricing_formula" valid:"jmespath"`
	UsageType      UsageType `yaml:"usage_type"`
	Units          SkuUnits  `yaml:"units"`
}

// SkuUnits stores SKU measurement units
type SkuUnits struct {
	Usage   string `yaml:"usage" valid:"not-empty"`
	Pricing string `yaml:"pricing" valid:"not-empty"`
}

// SkuResolving is info for resolve billing metric to specific SKU
type SkuResolving struct {
	Schemas         []string                          `yaml:"schemas" valid:"not-empty,uniq,each=name"`
	ResolvingPolicy string                            `yaml:"resolving_policy" valid:"jmespath"`
	ResolvingRules  []map[string]SkuResolveRuleOption `yaml:"resolving_rules" valid:"each=not-empty,each=keys=not-empty,each=each=valid"`
}

func (s Sku) Valid() error {
	errs := tools.ValidErrors{}
	if s.ResolvingPolicy != "" && len(s.ResolvingRules) != 0 {
		errs.Collect(errors.New("can not use resolving rules and policy simultaneously"))
	}
	return errs.Expose()
}

// SkuResolveRuleOption is special complex matching type for legacy resolving rules
type SkuResolveRuleOption struct {
	Null        bool
	EmptyString bool
	Str         string
	Number      float64
	IsTrue      bool
	IsFalse     bool
	ExistsRule  bool
	ExistsValue bool
}

func (o SkuResolveRuleOption) Valid() error {
	settedCount := 0
	inc := func(b bool) {
		if b {
			settedCount++
		}
	}
	inc(o.Null)
	inc(o.EmptyString)
	inc(o.Str != "")
	inc(o.Number != 0)
	inc(o.IsFalse)
	inc(o.IsTrue)
	inc(o.ExistsRule)
	if settedCount > 1 { // 0 is fine - empty number
		return errors.New("invalid resolving rule option")
	}
	return nil
}

func (o *SkuResolveRuleOption) UnmarshalYAML(unmarshal func(interface{}) error) error {
	{
		var val interface{}
		if err := unmarshal(&val); err != nil {
			return err
		}
		if val == nil {
			o.Null = true
			return nil
		}
	}
	{
		var val bool
		if err := unmarshal(&val); err == nil {
			if val {
				o.IsTrue = true
				return nil
			}
			o.IsFalse = true
			return nil
		}
	}
	{
		var val float64
		if err := unmarshal(&val); err == nil {
			o.Number = val
			return nil
		}
	}
	{
		var val string
		if err := unmarshal(&val); err == nil {
			o.Str = val
			o.EmptyString = val == ""
			return nil
		}
	}

	var val struct {
		Exists bool `yaml:"exists"`
	}
	if err := unmarshal(&val); err != nil {
		return err
	}
	o.ExistsRule = true
	o.ExistsValue = val.Exists
	return nil
}
