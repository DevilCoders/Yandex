package templateversions

import (
	"context"
	"testing"

	"github.com/stretchr/testify/suite"

	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/model"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/model/license"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/tests"
)

func TestUpdateAction(t *testing.T) {
	suite.Run(t, new(UpdateActionTestSuite))
}

type UpdateActionTestSuite struct {
	tests.BaseTestSuite
}

func (suite *UpdateActionTestSuite) TestUpdateOk() {
	ltv := &license.TemplateVersion{
		ID:         "version_id",
		TemplateID: "template_id",
		Period:     model.DefaultPeriod(),
		State:      license.PendingTemplateVersionState,
	}
	err := suite.Env.Adapters().DB().UpsertLicenseTemplateVersion(context.Background(), ltv, nil)
	suite.NoError(err)

	res, err := NewUpdateAction(suite.Env).Do(context.Background(), UpdateParams{
		ID:     ltv.ID,
		Price:  map[string]string{"RUB": "1000"},
		Period: "10_d",
		Name:   "name",
	})
	suite.Require().NoError(err)
	suite.Require().NotNil(res.TemplateVersion)

	ltv, err = suite.Env.Adapters().DB().GetLicenseTemplateVersionByID(context.Background(), ltv.ID, nil)

	suite.Require().NoError(err)
	suite.Require().NotNil(ltv)
	suite.Require().Equal("10_d", ltv.Period.String())
}

func (suite *UpdateActionTestSuite) TestUpdateError() {
	updateVersionResult, err := NewUpdateAction(suite.Env).Do(context.Background(), UpdateParams{
		ID:     "no_license_id",
		Price:  map[string]string{"RUB": "1000"},
		Period: "10_d",
		Name:   "License Template name",
	})

	suite.Require().Error(err)
	suite.Require().Nil(updateVersionResult)
}
