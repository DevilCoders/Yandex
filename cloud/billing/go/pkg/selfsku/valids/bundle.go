package valids

import (
	"fmt"
	"io/fs"

	"github.com/stretchr/testify/suite"

	"a.yandex-team.ru/cloud/billing/go/pkg/selfsku/bundler"
	"a.yandex-team.ru/cloud/billing/go/pkg/selfsku/tools"
)

type BundlesSuite struct {
	suite.Suite
	DataDir fs.FS

	bundles []string
	skus    map[string][]bundler.BundleSkus

	skuNames tools.StringSet
}

func (suite *BundlesSuite) SetupSuite() {
	var err error
	suite.skuNames = tools.NewStringSet()
	suite.skus = make(map[string][]bundler.BundleSkus)

	serviceSkus, err := bundler.LoadSkus(suite.DataDir)
	suite.Require().NoError(err)

	for _, s := range serviceSkus {
		for name := range s.Skus {
			suite.skuNames.Add(name)
		}
	}

	bundles, err := bundler.FindBundles(suite.DataDir)
	suite.Require().NoError(err)
	suite.bundles = bundles

	for _, b := range bundles {
		bs, err := bundler.LoadBundle(suite.DataDir, b)
		suite.Require().NoError(err)
		suite.skus[b] = append(suite.skus[b], bs...)
	}
}

func (suite *BundlesSuite) TestSkuConsistent() {
	for _, b := range suite.bundles {
		for _, skus := range suite.skus[b] {
			suite.Run(skus.LoadedFrom(), func() {
				for sku := range skus.BundleSkus {
					if !suite.skuNames.Contains(sku) {
						errMsg := fmt.Sprintf("'%s' inconsistent", sku)
						suite.Fail(errMsg, "bundle sku has no service sku definition")
					}
				}
			})
		}
	}
}

func (suite *BundlesSuite) TestSkuUnique() {
	for _, b := range suite.bundles {
		suite.Run(b, func() {
			foundSkus := uniqTester{}
			for _, skus := range suite.skus[b] {
				for sku := range skus.BundleSkus {
					if uniq, prev := foundSkus.check(sku, skus.LoadedFrom()); !uniq {
						errMsg := fmt.Sprintf("'%s' at '%s' duplicated", sku, skus.LoadedFrom())
						suite.Fail(errMsg, "previously found at '%s'", prev)
					}
				}
			}
		})
	}
}
