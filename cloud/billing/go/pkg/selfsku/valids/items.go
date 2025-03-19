package valids

import (
	"fmt"
	"io/fs"

	"github.com/stretchr/testify/suite"

	"a.yandex-team.ru/cloud/billing/go/pkg/selfsku/bundler"
)

type ItemsSuite struct {
	suite.Suite
	DataDir fs.FS

	services    []bundler.Service
	schemas     []bundler.TagChecks
	units       []bundler.UnitConversion
	serviceSkus []bundler.ServiceSkus
	bundleSkus  []bundler.BundleSkus
}

func (suite *ItemsSuite) SetupSuite() {
	var err error
	suite.services, err = bundler.LoadServices(suite.DataDir)
	suite.Require().NoError(err)

	suite.schemas, err = bundler.LoadSchemas(suite.DataDir)
	suite.Require().NoError(err)

	suite.units, err = bundler.LoadUnits(suite.DataDir)
	suite.Require().NoError(err)

	suite.serviceSkus, err = bundler.LoadSkus(suite.DataDir)
	suite.Require().NoError(err)

	bundles, err := bundler.FindBundles(suite.DataDir)
	suite.Require().NoError(err)

	for _, b := range bundles {
		bs, err := bundler.LoadBundle(suite.DataDir, b)
		suite.Require().NoError(err)
		suite.bundleSkus = append(suite.bundleSkus, bs...)
	}
}

func (suite *ItemsSuite) TestServices() {
	for _, s := range suite.services {
		suite.Run(s.LoadedFrom(), func() {
			suite.NoError(s.Valid())
		})
	}
}

func (suite *ItemsSuite) TestSchemas() {
	for _, s := range suite.schemas {
		suite.Run(s.LoadedFrom(), func() {
			suite.NoError(s.Valid())
		})
	}
}

func (suite *ItemsSuite) TestUnits() {
	for _, u := range suite.units {
		name := fmt.Sprintf("%s:%s-%s", u.LoadedFrom(), u.SrcUnit, u.DstUnit)
		suite.Run(name, func() {
			suite.NoError(u.Valid())
		})
	}
}

func (suite *ItemsSuite) TestSkus() {
	for _, s := range suite.serviceSkus {
		suite.Run(s.LoadedFrom(), func() {
			suite.NoError(s.Valid())
		})
	}
}

func (suite *ItemsSuite) TestBundles() {
	for _, s := range suite.bundleSkus {
		suite.Run(s.LoadedFrom(), func() {
			suite.NoError(s.Valid())
		})
	}
}
