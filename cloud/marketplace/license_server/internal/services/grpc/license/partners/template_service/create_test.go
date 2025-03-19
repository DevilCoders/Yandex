package templateservice

import (
	"context"
	"testing"

	"github.com/stretchr/testify/mock"
	"github.com/stretchr/testify/suite"

	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/marketplace/licenseserver/v1/license"
	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/marketplace/licenseserver/v1/license/partners"
	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/operation"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/model"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/tests"
	"a.yandex-team.ru/cloud/marketplace/pkg/clients/marketplace-private"
)

func TestCreateGrpc(t *testing.T) {
	suite.Run(t, new(CreateGrpcTestSuite))
}

type CreateGrpcTestSuite struct {
	tests.BaseTestSuite

	service *templateService
}

func (suite *CreateGrpcTestSuite) SetupTest() {
	suite.service = NewTemplateService(suite.Env)
}

func (suite *CreateGrpcTestSuite) TestCreatingOk() {
	suite.MarketplaceMock.On("GetTariff", mock.Anything).Return(&marketplace.Tariff{
		ID:          "tariff_id",
		PublisherID: "test_publisher_id",
		ProductID:   "test_product_id",
		State:       model.ActiveTariffState,
		Type:        model.PAYGTariffType,
	}, nil)
	suite.DefaultAuthMock.On("Token").Return("token", nil)

	resOp, err := suite.service.Create(context.Background(), &partners.CreateTemplateRequest{
		PublisherId: "publisher_id",
		ProductId:   "product_id",
		TariffId:    "tariff_id",
	})

	suite.Require().NoError(err)

	suite.Require().NotNil(resOp)
	suite.Require().Equal(true, resOp.Done)

	ltGrpc := new(license.Template)
	err = resOp.Result.(*operation.Operation_Response).Response.UnmarshalTo(ltGrpc)

	suite.Require().NoError(err)

	lt, err := suite.Env.Adapters().DB().GetLicenseTemplateByID(context.Background(), ltGrpc.Id, nil)

	suite.Require().NoError(err)
	suite.Require().NotNil(lt)
}
