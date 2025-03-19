package bundler

import (
	"fmt"
	"io"
	"os"
	"time"

	"gopkg.in/yaml.v2"

	"a.yandex-team.ru/cloud/billing/go/pkg/selfsku/applicables"
	"a.yandex-team.ru/cloud/billing/go/pkg/selfsku/origin"
)

func MigrateSkuFile(path string, service string) (origin.BundleSkus, origin.ServiceSkus, error) {
	f, err := os.Open(path)
	if err != nil {
		return nil, origin.ServiceSkus{}, fmt.Errorf("%s:%w", path, err)
	}
	defer func() { _ = f.Close() }()

	return MigrateSkuData(f, service)
}

func MigrateSkuData(data io.Reader, service string) (origin.BundleSkus, origin.ServiceSkus, error) {
	var items []applicables.Sku

	dec := yaml.NewDecoder(data)
	if err := dec.Decode(&items); err != nil {
		return nil, origin.ServiceSkus{}, err
	}

	bndl := make(origin.BundleSkus)
	srv := origin.ServiceSkus{
		Service: service,
		Skus:    make(map[string]origin.Sku),
	}

	for _, i := range items {
		bs := origin.BundleSku{
			ID: i.ID,
		}
		for _, p := range i.PricingVersions {
			price := origin.SkuPrice{
				StartDate: origin.YamlTime(time.Unix(int64(p.EffectiveTime), 0)),
			}
			switch len(p.PricingExpression.Rates) {
			case 1:
				price.Price = p.PricingExpression.Rates[0].UnitPrice
			default:
				for _, r := range p.PricingExpression.Rates {
					price.Rates = append(price.Rates, origin.PriceRate{
						Quantity: r.StartPricingQuantity,
						Price:    r.UnitPrice,
					})
				}
			}
			bs.Prices = append(bs.Prices, price)
		}
		bndl[i.Name] = bs

		rptService := fmt.Sprintf("<service with id %s>", i.Labels.RealServiceID)
		if i.Labels.RealServiceID == i.ServiceID {
			rptService = service
		}

		s := origin.Sku{
			SkuNames: origin.SkuNames{
				Ru: i.Translations.Ru.Name,
				En: i.Translations.En.Name,
			},
			SkuAnalytics: origin.SkuAnalytics{
				ReportingService: fmt.Sprintf("%s/%s", rptService, i.Labels.Subservice),
				Private:          i.Labels.Visibility == "PRIVATE",
			},
			SkuUsage: origin.SkuUsage{
				PricingFormula: i.Formula,
				Units: origin.SkuUnits{
					Usage:   i.UsageUnit,
					Pricing: i.PricingUnit,
				},
			},
			SkuResolving: origin.SkuResolving{
				Schemas:         i.Schemas,
				ResolvingPolicy: i.ResolvingPolicy,
			},
		}
		_ = s.UsageType.UnmarshalText([]byte(i.UsageType))
		if s.PricingFormula == "usage.quantity" {
			s.PricingFormula = ""
		}

		for _, rr := range i.ResolvingRules {
			orule := make(map[string]origin.SkuResolveRuleOption)
			for path, rule := range rr {
				orule[path] = origin.SkuResolveRuleOption{
					Null:        rule.Null,
					EmptyString: rule.EmptyString,
					Str:         rule.Str,
					Number:      rule.Number,
					IsTrue:      rule.IsTrue,
					IsFalse:     rule.IsFalse,
					ExistsRule:  rule.ExistsRule,
					ExistsValue: rule.ExistsValue,
				}
			}
			s.ResolvingRules = append(s.ResolvingRules, orule)
		}
		srv.Skus[i.Name] = s
	}
	return bndl, srv, nil
}
