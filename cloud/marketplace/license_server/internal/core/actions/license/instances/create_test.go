package instances

import (
	"context"
	"testing"
	"time"

	"github.com/stretchr/testify/mock"
	"github.com/stretchr/testify/suite"

	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/actions/license/templates"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/model"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/model/license"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/tests"
	"a.yandex-team.ru/cloud/marketplace/pkg/clients/marketplace-private"
)

func TestCreateAction(t *testing.T) {
	suite.Run(t, new(CreateActionTestSuite))
}

type CreateActionTestSuite struct {
	tests.BaseTestSuite
}

func (suite *CreateActionTestSuite) TestCreatingOk() {
	period, err := model.NewPeriod("10_d")
	suite.Require().NoError(err)
	lt := &license.Template{
		ID:                "id",
		TemplateVersionID: "template_version_id",
		ProductID:         "product_id",
		TariffID:          "tariff_id",
		Name:              "name",
		Period:            period,
		State:             license.ActiveTemplateState,
	}
	err = suite.Env.Adapters().DB().UpsertLicenseTemplate(context.Background(), lt, nil)
	suite.Require().NoError(err)

	cloudID := "cloud_id"
	createInstance, err := NewCreateAction(suite.Env).Do(context.Background(), CreateParams{
		TemplateID: lt.ID,
		CloudID:    cloudID,
	})

	suite.Require().NoError(err)
	suite.Require().NotNil(createInstance.Instance)

	li, err := suite.Env.Adapters().DB().GetLicenseInstanceByID(context.Background(), createInstance.Instance.ID, nil)
	suite.Require().NoError(err)
	suite.Require().NotNil(li)
	suite.Require().Equal(cloudID, li.CloudID)
	suite.Require().Equal(createInstance.Instance.StartTime.Add(10*24*time.Hour), li.EndTime)
	suite.Require().Equal(lt.ID, li.TemplateID)
}

func (suite *CreateActionTestSuite) TestCreateError() {
	suite.MarketplaceMock.On("GetTariff", mock.Anything).Return(&marketplace.Tariff{
		ID:          "tariff_id",
		PublisherID: "test_publisher_id",
		ProductID:   "test_product_id",
		State:       model.ActiveTariffState,
		Type:        model.PAYGTariffType,
	}, nil)
	suite.DefaultAuthMock.On("Token").Return("token", nil)
	createResult, err := templates.NewCreateAction(suite.Env).Do(context.Background(), templates.CreateParams{
		PublisherID: "publisher_id",
		ProductID:   "product_id",
		TariffID:    "tariff_id",
	})

	suite.Require().NoError(err)
	suite.Require().NotNil(createResult)
	suite.Require().NotEmpty(createResult.Template.ID)

	cloudID := "cloud_id"

	createInstance, err := NewCreateAction(suite.Env).Do(context.Background(), CreateParams{
		TemplateID: createResult.Template.ID,
		CloudID:    cloudID,
	})

	suite.Require().Error(err)
	suite.Require().Nil(createInstance)
}
