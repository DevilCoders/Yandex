package templates

import (
	"context"
	"testing"

	"github.com/stretchr/testify/suite"

	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/model"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/model/license"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/tests"
)

func TestDeprecateAction(t *testing.T) {
	suite.Run(t, new(DeprecateActionTestSuite))
}

type DeprecateActionTestSuite struct {
	tests.BaseTestSuite
}

func (suite *DeprecateActionTestSuite) TestDeprecateOk() {
	lt := &license.Template{
		ID:     "id",
		Period: model.DefaultPeriod(),
		State:  license.PendingTemplateState,
	}
	err := suite.Env.Adapters().DB().UpsertLicenseTemplate(context.Background(), lt, nil)
	suite.NoError(err)

	res, err := NewDeprecateAction(suite.Env).Do(context.Background(), DeprecateParams{ID: lt.ID})
	suite.Require().NoError(err)
	suite.Require().NotNil(res.Template)

	lt, err = suite.Env.Adapters().DB().GetLicenseTemplateByID(context.Background(), lt.ID, nil)
	suite.NoError(err)
	suite.NotNil(lt)
	suite.Require().Equal(license.DeprecatedTemplateState, lt.State)
}

func (suite *DeprecateActionTestSuite) TestDeprecateWithVersionOk() {
	lt := &license.Template{
		ID:     "id",
		Period: model.DefaultPeriod(),
		State:  license.PendingTemplateState,
	}
	err := suite.Env.Adapters().DB().UpsertLicenseTemplate(context.Background(), lt, nil)
	suite.NoError(err)
	ltv := &license.TemplateVersion{
		ID:         "version_id",
		TemplateID: lt.ID,
		Period:     model.DefaultPeriod(),
		State:      license.PendingTemplateVersionState,
	}
	err = suite.Env.Adapters().DB().UpsertLicenseTemplateVersion(context.Background(), ltv, nil)
	suite.NoError(err)

	res, err := NewDeprecateAction(suite.Env).Do(context.Background(), DeprecateParams{ID: lt.ID})
	suite.Require().NoError(err)
	suite.Require().NotNil(res.Template)

	lt, err = suite.Env.Adapters().DB().GetLicenseTemplateByID(context.Background(), lt.ID, nil)
	suite.NoError(err)
	suite.NotNil(lt)
	suite.Require().Equal(license.DeprecatedTemplateState, lt.State)

	ltv, err = suite.Env.Adapters().DB().GetLicenseTemplateVersionByID(context.Background(), ltv.ID, nil)
	suite.Require().NoError(err)
	suite.Require().NotNil(ltv)
	suite.Require().Equal(license.DeprecatedTemplateVersionState, ltv.State)
}

func (suite *DeprecateActionTestSuite) TestDeprecatingError() {
	deprecateResult, err := NewDeprecateAction(suite.Env).Do(context.Background(), DeprecateParams{ID: "not_found_id"})
	suite.Require().Error(err)
	suite.Require().Nil(deprecateResult)
}
