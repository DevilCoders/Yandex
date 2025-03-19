package bundler

import (
	"strings"

	"a.yandex-team.ru/cloud/billing/go/pkg/selfsku/applicables"
	"a.yandex-team.ru/cloud/billing/go/pkg/selfsku/origin"
	"a.yandex-team.ru/cloud/billing/go/pkg/selfsku/tools"
)

func (b *Bundle) Units() (result []applicables.Unit) {
	for _, u := range b.units {
		result = append(result, applicables.Unit{
			SrcUnit: u.SrcUnit,
			DstUnit: u.DstUnit,
			Factor:  u.Factor,
		})
	}
	return
}

func (b *Bundle) Services() (result []applicables.Service) {
	for _, s := range b.services {
		result = append(result, applicables.Service{
			ID:          s.ID,
			Name:        s.Name,
			Description: s.Description,
			Group:       s.Group,
		})
	}
	return
}

func (b *Bundle) Schemas() (result []applicables.Schema) {
	schemas := tools.NewStringSet()
	for _, srvSku := range b.skus {
		for _, sku := range srvSku.Skus {
			schemas.Add(sku.Schemas...)
		}
	}
	for sch := range schemas {
		tc := b.schemas[sch]
		result = append(result, applicables.Schema{
			Name: sch,
			Tags: applicables.SchemaTags{
				Required: tc.Required,
				Optional: tc.Optional,
			},
		})
	}
	return
}

func (b *Bundle) Sku(env string) (result []applicables.Sku) {
	bskus := b.bundleSkus[env]

	idx := b.index()

	for _, srvSku := range b.skus {
		srvNum := idx.servicesIdx[srvSku.Service]
		srv := b.services[srvNum]
		for name, sku := range srvSku.Skus {
			resolvingPolicy := "`true`"
			switch {
			case sku.ResolvingPolicy != "":
				resolvingPolicy = sku.ResolvingPolicy
			case len(sku.ResolvingRules) > 0:
				resolvingPolicy = ""
			}
			formula := "usage.quantity"
			if sku.PricingFormula != "" {
				formula = sku.PricingFormula
			}
			rptSrvName, subservice := parseReportingService(sku.ReportingService)
			rptSrvNum := idx.servicesIdx[rptSrvName]
			reportService := b.services[rptSrvNum]
			visibility := "PUBLIC"
			if sku.Private {
				visibility = "PRIVATE"
			}
			created, prices := convertPrices(bskus[name].Prices)

			result = append(result, applicables.Sku{
				ID:              bskus[name].ID,
				Name:            name,
				ServiceID:       srv.ID,
				Schemas:         sku.Schemas,
				ResolvingPolicy: resolvingPolicy,
				ResolvingRules:  convertResolvingRules(sku.ResolvingRules),
				Formula:         formula,
				UsageUnit:       sku.Units.Usage,
				PricingUnit:     sku.Units.Pricing,
				UsageType:       sku.UsageType.String(),
				Labels: applicables.SkuLabels{
					RealServiceID: reportService.ID,
					Subservice:    subservice,
					Visibility:    visibility,
				},
				CreatedAt:       created,
				PricingVersions: prices,
				Translations: applicables.Translations{
					Ru: applicables.TranslationName{Name: sku.Ru},
					En: applicables.TranslationName{Name: sku.En},
				},
			})
		}
	}
	return
}

func convertResolvingRules(inp []map[string]origin.SkuResolveRuleOption) (outp []applicables.ResolvingRule) {
	for _, rr := range inp {
		or := make(applicables.ResolvingRule)
		for k, v := range rr {
			or[k] = applicables.ResolvingRuleOption{
				Null:        v.Null,
				EmptyString: v.EmptyString,
				Str:         v.Str,
				Number:      v.Number,
				IsTrue:      v.IsTrue,
				IsFalse:     v.IsFalse,
				ExistsRule:  v.ExistsRule,
				ExistsValue: v.ExistsValue,
			}
		}
		outp = append(outp, or)
	}
	return
}

func parseReportingService(name string) (service, subservice string) {
	parts := strings.SplitN(name, "/", 2)
	service = parts[0]
	if len(parts) > 1 {
		subservice = parts[1]
	}
	return
}

func convertPrices(inp []origin.SkuPrice) (created int, outp []applicables.PricingVersion) {
	for i, p := range inp {
		if i == 0 {
			created = p.StartDate.Unix()
		}
		o := applicables.PricingVersion{
			EffectiveTime: p.StartDate.Unix(),
		}

		switch len(p.Rates) {
		case 0:
			o.PricingExpression.Rates = []applicables.PricingRate{{UnitPrice: p.Price}}
		default:
			for _, r := range p.Rates {
				o.PricingExpression.Rates = append(o.PricingExpression.Rates, applicables.PricingRate{
					StartPricingQuantity: r.Quantity,
					UnitPrice:            r.Price,
				})
			}
		}
		outp = append(outp, o)
	}
	return
}
