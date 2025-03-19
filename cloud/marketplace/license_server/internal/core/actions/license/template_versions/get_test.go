package templateversions

import (
	"context"
	"testing"

	"github.com/stretchr/testify/suite"

	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/model"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/model/license"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/tests"
)

func TestGetAction(t *testing.T) {
	suite.Run(t, new(GetActionTestSuite))
}

type GetActionTestSuite struct {
	tests.BaseTestSuite
}

func (suite *GetActionTestSuite) TestGetOk() {
	ltv := &license.TemplateVersion{
		ID:         "version_id",
		TemplateID: "template_id",
		Period:     model.DefaultPeriod(),
		State:      license.DeprecatedTemplateVersionState,
	}
	err := suite.Env.Adapters().DB().UpsertLicenseTemplateVersion(context.Background(), ltv, nil)
	suite.NoError(err)

	res, err := NewGetAction(suite.Env).Do(context.Background(), GetParams{ID: ltv.ID})
	suite.Require().NoError(err)
	suite.Require().NotNil(res.TemplateVersion)
	suite.Require().Equal(ltv.ID, res.TemplateVersion.ID)
}

func (suite *GetActionTestSuite) TestGetError() {
	getVersionResult, err := NewGetAction(suite.Env).Do(context.Background(), GetParams{ID: "no_id"})
	suite.Require().Error(err)
	suite.Require().Nil(getVersionResult)
}
