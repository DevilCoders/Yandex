package tooling

import (
	"testing"

	"github.com/stretchr/testify/suite"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/pkg/timetool"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling/features"
)

type featuresSuite struct {
	baseSuite
}

func TestFeaturesSuite(t *testing.T) {
	suite.Run(t, new(featuresSuite))
}

func (suite *featuresSuite) TestDefaultFeatures() {
	suite.Require().False(Features(suite.ctx).DropDuplicates())
	suite.Require().Equal(Features(suite.ctx).LocalTimezone(), timetool.DefaultTz())
}

func (suite *featuresSuite) TestDefaultUpdated() {
	flags := features.Default()
	flags.Set(features.DropDuplicates(true))
	features.SetDefault(flags)

	suite.Require().True(Features(suite.ctx).DropDuplicates())
}
