package templates

import (
	"context"
	"testing"

	"github.com/stretchr/testify/suite"

	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/model"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/model/license"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/tests"
)

func TestDeleteAction(t *testing.T) {
	suite.Run(t, new(DeleteActionTestSuite))
}

type DeleteActionTestSuite struct {
	tests.BaseTestSuite
}

func (suite *DeleteActionTestSuite) TestDeletingOk() {
	lt := &license.Template{
		ID:     "id",
		Period: model.DefaultPeriod(),
		State:  license.PendingTemplateState,
	}
	err := suite.Env.Adapters().DB().UpsertLicenseTemplate(context.Background(), lt, nil)
	suite.NoError(err)

	err = NewDeleteAction(suite.Env).Do(context.Background(), DeleteParams{ID: lt.ID})
	suite.Require().NoError(err)

	lt, err = suite.Env.Adapters().DB().GetLicenseTemplateByID(context.Background(), lt.ID, nil)
	suite.Error(err)
	suite.Nil(lt)
}

func (suite *DeleteActionTestSuite) TestDeletingAndDeprecateVersion() {
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

	err = NewDeleteAction(suite.Env).Do(context.Background(), DeleteParams{ID: lt.ID})
	suite.Require().NoError(err)

	lt, err = suite.Env.Adapters().DB().GetLicenseTemplateByID(context.Background(), lt.ID, nil)
	suite.Require().Error(err)
	suite.Require().Nil(lt)

	ltv, err = suite.Env.Adapters().DB().GetLicenseTemplateVersionByID(context.Background(), ltv.ID, nil)
	suite.Require().NoError(err)
	suite.Require().NotNil(ltv)
	suite.Require().Equal(license.DeprecatedTemplateVersionState, ltv.State)
}

func (suite *DeleteActionTestSuite) TestDeletingError() {
	err := NewDeleteAction(suite.Env).Do(context.Background(), DeleteParams{ID: "not_found_id"})
	suite.Require().Error(err)
}
