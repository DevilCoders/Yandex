package templateversions

import (
	"context"
	"testing"

	"github.com/stretchr/testify/mock"
	"github.com/stretchr/testify/suite"

	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/model"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/model/license"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/tests"
	"a.yandex-team.ru/cloud/marketplace/pkg/clients/billing-private"
)

func TestApplyAction(t *testing.T) {
	suite.Run(t, new(ApplyActionTestSuite))
}

type ApplyActionTestSuite struct {
	tests.BaseTestSuite
}

func (suite *ApplyActionTestSuite) TestApplyActionOk() {
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

	suite.BillingMock.On("CreateSku", mock.Anything).Return(billing.Sku{ID: "id"}, nil)
	suite.DefaultAuthMock.On("Token").Return("token", nil)

	res, err := NewApplyAction(suite.Env).Do(context.Background(), ApplyParams{
		ID: ltv.ID,
	})

	suite.Require().NoError(err)
	suite.Require().NotNil(res.Template)
	suite.Require().NotNil(res.TemplateVersion)
	suite.Require().Equal(license.ActiveTemplateState, res.Template.State)
	suite.Require().Equal(license.ActiveTemplateVersionState, res.TemplateVersion.State)
}

func (suite *ApplyActionTestSuite) TestApplyActionError() {
	suite.BillingMock.On("CreateSku", mock.Anything).Return(billing.Sku{ID: "id"}, nil)
	suite.DefaultAuthMock.On("Token").Return("token", nil)

	applyVersionResult, err := NewApplyAction(suite.Env).Do(context.Background(), ApplyParams{
		ID: "no_id",
	})

	suite.Require().Error(err)
	suite.Require().Nil(applyVersionResult)
}
