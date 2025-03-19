package templateversions

import (
	"context"
	"testing"

	"github.com/stretchr/testify/suite"

	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/model"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/model/license"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/tests"
)

func TestCreateAction(t *testing.T) {
	suite.Run(t, new(CreateActionTestSuite))
}

type CreateActionTestSuite struct {
	tests.BaseTestSuite
}

func (suite *CreateActionTestSuite) TestCreatingOk() {
	lt := &license.Template{
		ID:     "id",
		Period: model.DefaultPeriod(),
		State:  license.PendingTemplateState,
	}
	err := suite.Env.Adapters().DB().UpsertLicenseTemplate(context.Background(), lt, nil)
	suite.NoError(err)

	createVersionResult, err := NewCreateAction(suite.Env).Do(context.Background(), CreateParams{
		TemplateID: lt.ID,
		Price:      map[string]string{"RUB": "1000"},
		Period:     "10_d",
		Name:       "License Template name",
	})

	suite.Require().NoError(err)
	suite.Require().NotNil(createVersionResult)
	suite.Require().Equal(lt.ID, createVersionResult.TemplateVersion.TemplateID)
}

func (suite *CreateActionTestSuite) TestCreateSecondPendingError() {
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

	createVersionResultNil, err := NewCreateAction(suite.Env).Do(context.Background(), CreateParams{
		TemplateID: lt.ID,
		Price:      map[string]string{"RUB": "1000"},
		Period:     "10_d",
		Name:       "License Template name",
	})

	suite.Require().Error(err)
	suite.Require().Nil(createVersionResultNil)
}

func (suite *CreateActionTestSuite) TestCreatingError() {
	createVersionResult, err := NewCreateAction(suite.Env).Do(context.Background(), CreateParams{
		TemplateID: "no_license_id",
		Price:      map[string]string{"RUB": "1000"},
		Period:     "10_d",
		Name:       "License Template name",
	})

	suite.Require().Error(err)
	suite.Require().Nil(createVersionResult)
}
