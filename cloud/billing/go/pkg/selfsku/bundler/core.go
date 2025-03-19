package bundler

import (
	"strings"

	"a.yandex-team.ru/cloud/billing/go/pkg/selfsku/origin"
)

type Bundle struct {
	units    []origin.UnitConversion
	services []origin.Service
	skus     []origin.ServiceSkus
	schemas  origin.TagChecks

	bundleSkus map[string]origin.BundleSkus
}

func (b *Bundle) FilterForEnv(name string) *Bundle {
	result := Bundle{
		bundleSkus: map[string]origin.BundleSkus{name: b.bundleSkus[name]},
	}

	if _, ok := b.bundleSkus[name]; !ok {
		return &result
	}

	idx := b.index()

	skuNames := map[string]markerType{}
	usedSkus := map[int]markerType{}
	for skuName := range result.bundleSkus[name] {
		skuNames[skuName] = marker
		skuNum := idx.skusIdx[skuName]
		usedSkus[skuNum] = marker
	}

	usedSrv := map[int]markerType{}
	usedUnits := map[int]markerType{}
	for i := range usedSkus {
		skus := b.skus[i]

		srvNum := idx.servicesIdx[skus.Service]
		usedSrv[srvNum] = marker

		newSkus := origin.ServiceSkus{
			Service: skus.Service,
			Skus:    make(map[string]origin.Sku),
		}
		for name, sku := range skus.Skus {
			if _, used := skuNames[name]; !used {
				continue
			}
			unitsNum := idx.unitsIdx[unitsPair{src: sku.Units.Usage, dst: sku.Units.Pricing}]
			usedUnits[unitsNum] = marker

			realSrv := strings.SplitN(sku.ReportingService, "/", 2)[0]
			realSrvNum := idx.servicesIdx[realSrv]
			usedSrv[realSrvNum] = marker

			for _, sch := range sku.Schemas {
				if _, ok := result.schemas[sch]; ok {
					continue
				}
				if tc, ok := b.schemas[sch]; ok {
					result.schemas[sch] = tc
				}
			}

			newSkus.Skus[name] = sku
		}
		result.skus = append(result.skus, newSkus)
	}

	for i := range usedSrv {
		result.services = append(result.services, b.services[i])
	}
	for i := range usedUnits {
		result.units = append(result.units, b.units[i])
	}

	return &result
}

type bundleIndex struct {
	bundle      *Bundle
	unitsIdx    map[unitsPair]int
	servicesIdx map[string]int
	skusIdx     map[string]int
}

func (b *Bundle) index() (bi bundleIndex) {
	bi.bundle = b
	bi.unitsIdx = map[unitsPair]int{}
	for i, u := range b.units {
		k := unitsPair{src: u.SrcUnit, dst: u.DstUnit}
		bi.unitsIdx[k] = i
	}

	bi.servicesIdx = map[string]int{}
	for i, s := range b.services {
		bi.servicesIdx[s.Name] = i
	}

	bi.skusIdx = map[string]int{}
	for i, s := range b.skus {
		for name := range s.Skus {
			bi.skusIdx[name] = i
		}
	}
	return
}

type unitsPair struct {
	src string
	dst string
}

type markerType struct{}

var marker markerType
