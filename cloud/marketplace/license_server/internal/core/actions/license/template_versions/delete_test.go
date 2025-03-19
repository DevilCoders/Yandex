package templateversions

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
	ltv := &license.TemplateVersion{
		ID:         "version_id",
		TemplateID: "template_id",
		Period:     model.DefaultPeriod(),
		State:      license.DeprecatedTemplateVersionState,
	}
	err := suite.Env.Adapters().DB().UpsertLicenseTemplateVersion(context.Background(), ltv, nil)
	suite.NoError(err)

	err = NewDeleteAction(suite.Env).Do(context.Background(), DeleteParams{ID: ltv.ID})
	suite.Require().NoError(err)

	ltv, err = suite.Env.Adapters().DB().GetLicenseTemplateVersionByID(context.Background(), ltv.ID, nil)

	suite.Require().Error(err)
	suite.Require().Nil(ltv)
}

func (suite *DeleteActionTestSuite) TestDeletingError() {
	err := NewDeleteAction(suite.Env).Do(context.Background(), DeleteParams{ID: "no_id"})
	suite.Require().Error(err)
}
