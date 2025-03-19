package templates

import (
	"context"
	"testing"

	"github.com/stretchr/testify/mock"
	"github.com/stretchr/testify/suite"

	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/model"
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
	suite.MarketplaceMock.On("GetTariff", mock.Anything).Return(&marketplace.Tariff{
		ID:          "tariff_id",
		PublisherID: "test_publisher_id",
		ProductID:   "test_product_id",
		State:       model.ActiveTariffState,
		Type:        model.PAYGTariffType,
	}, nil)
	suite.DefaultAuthMock.On("Token").Return("token", nil)

	actionResult, err := NewCreateAction(suite.Env).Do(context.Background(), CreateParams{
		PublisherID: "publisher_id",
		ProductID:   "product_id",
		TariffID:    "tariff_id",
	})

	suite.Require().NoError(err)
	suite.Require().NotNil(actionResult)
	suite.Require().NotEmpty(actionResult.Template.ID)
	suite.Require().Equal("test_product_id", actionResult.Template.ProductID)
}
