package valids

import (
	"fmt"
	"io/fs"
	"strings"

	"github.com/stretchr/testify/suite"

	"a.yandex-team.ru/cloud/billing/go/pkg/selfsku/bundler"
	"a.yandex-team.ru/cloud/billing/go/pkg/selfsku/tools"
)

type DefinitionsSuite struct {
	suite.Suite
	DataDir fs.FS

	services    []bundler.Service
	schemas     []bundler.TagChecks
	units       []bundler.UnitConversion
	serviceSkus []bundler.ServiceSkus
}

func (suite *DefinitionsSuite) SetupSuite() {
	var err error
	suite.services, err = bundler.LoadServices(suite.DataDir)
	suite.Require().NoError(err)

	suite.schemas, err = bundler.LoadSchemas(suite.DataDir)
	suite.Require().NoError(err)

	suite.units, err = bundler.LoadUnits(suite.DataDir)
	suite.Require().NoError(err)

	suite.serviceSkus, err = bundler.LoadSkus(suite.DataDir)
	suite.Require().NoError(err)
}

func (suite *DefinitionsSuite) TestServicesUnique() {
	foundNames := uniqTester{}
	foundIDs := uniqTester{}
	for _, s := range suite.services {
		if uniq, prev := foundNames.check(s.Name, s.LoadedFrom()); !uniq {
			errMsg := fmt.Sprintf("name '%s' at '%s' duplicated", s.Name, s.LoadedFrom())
			suite.Fail(errMsg, "previously found at '%s'", prev)
		}
		if uniq, prev := foundIDs.check(s.ID, s.LoadedFrom()); !uniq {
			errMsg := fmt.Sprintf("id '%s' at '%s' duplicated", s.ID, s.LoadedFrom())
			suite.Fail(errMsg, "previously found at '%s'", prev)
		}
	}
}

func (suite *DefinitionsSuite) TestUnitsUnique() {
	units := map[stringPair]string{}
	for _, u := range suite.units {
		key := stringPair{u.SrcUnit, u.DstUnit}
		if prev, ok := units[key]; ok {
			errMsg := fmt.Sprintf("units '%s'->'%s' at '%s' duplicated", u.SrcUnit, u.DstUnit, u.LoadedFrom())
			suite.Fail(errMsg, "previously found at '%s'", prev)
			continue
		}
		units[key] = u.LoadedFrom()
	}
}

func (suite *DefinitionsSuite) TestSkusUnique() {
	foundSkus := uniqTester{}
	foundRuNames := uniqTester{}
	foundEnNames := uniqTester{}
	for _, s := range suite.serviceSkus {
		for name, sku := range s.Skus {
			if uniq, prev := foundSkus.check(name, s.LoadedFrom()); !uniq {
				errMsg := fmt.Sprintf("sku '%s' at '%s' duplicated", name, s.LoadedFrom())
				suite.Fail(errMsg, "previously found at '%s'", prev)
			}
			if uniq, prev := foundRuNames.check(sku.Ru, s.LoadedFrom()); !uniq {
				errMsg := fmt.Sprintf("sku ru name '%s' at '%s' duplicated", sku.Ru, s.LoadedFrom())
				suite.Fail(errMsg, "previously found at '%s'", prev)
			}
			if uniq, prev := foundEnNames.check(sku.En, s.LoadedFrom()); !uniq {
				errMsg := fmt.Sprintf("sku en name '%s' at '%s' duplicated", sku.En, s.LoadedFrom())
				suite.Fail(errMsg, "previously found at '%s'", prev)
			}
		}
	}
}

func (suite *DefinitionsSuite) TestSkuHasService() {
	services := tools.NewStringSet()
	for _, s := range suite.services {
		services.Add(s.Name)
	}

	for _, s := range suite.serviceSkus {
		if !services.Contains(s.Service) {
			suite.Fail(fmt.Sprintf("service '%s' at '%s' has no definition", s.Service, s.LoadedFrom()))
		}
		for name, sku := range s.Skus {
			repService := strings.SplitN(sku.ReportingService, "/", 2)[0]
			if !services.Contains(repService) {
				errMsg := fmt.Sprintf("sku '%s' at '%s' has no reporting service definition", name, s.LoadedFrom())
				suite.Fail(errMsg, "reporting_service='%s'", sku.ReportingService)
			}
		}
	}
}

func (suite *DefinitionsSuite) TestSkuUnitsConvertable() {
	units := map[stringPair]struct{}{}
	for _, u := range suite.units {
		units[stringPair{u.SrcUnit, u.DstUnit}] = setMark
	}

	for _, s := range suite.serviceSkus {
		for name, sku := range s.Skus {
			if sku.Units.Usage == sku.Units.Pricing {
				continue
			}

			if _, found := units[stringPair{sku.Units.Usage, sku.Units.Pricing}]; !found {
				errMsg := fmt.Sprintf("sku '%s' at '%s' units not convertable", name, s.LoadedFrom())
				suite.Fail(errMsg, "usage '%s' -> pricing '%s'", sku.Units.Usage, sku.Units.Pricing)
			}
		}
	}
}

func (suite *DefinitionsSuite) TestSchemasUnique() {
	found := uniqTester{}
	for _, sch := range suite.schemas {
		for name := range sch.TagChecks {
			if uniq, prev := found.check(name, sch.LoadedFrom()); !uniq {
				errMsg := fmt.Sprintf("schema '%s' at '%s' duplicated", name, sch.LoadedFrom())
				suite.Fail(errMsg, "previously found at '%s'", prev)
			}
		}
	}
}

func (suite *DefinitionsSuite) TestSchemasUsed() {
	skuSchemas := tools.NewStringSet()
	for _, s := range suite.serviceSkus {
		for _, sku := range s.Skus {
			skuSchemas.Add(sku.Schemas...)
		}
	}

	for _, sch := range suite.schemas {
		for name := range sch.TagChecks {
			if !skuSchemas.Contains(name) {
				suite.Fail(fmt.Sprintf("schema '%s' at '%s' unused", name, sch))
			}
		}
	}
}
