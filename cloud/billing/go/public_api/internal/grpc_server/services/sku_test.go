package services

import (
	"testing"

	"github.com/stretchr/testify/mock"
	"github.com/stretchr/testify/suite"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/status"
	timestamppb "google.golang.org/protobuf/types/known/timestamppb"

	"a.yandex-team.ru/cloud/billing/go/public_api/internal/console"
	"a.yandex-team.ru/cloud/billing/go/public_api/internal/grpc_server/services/mocks"
	billing_grpc "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/billing/v1"
)

type skuMocks struct {
	client *mocks.Client
}

func (suite *skuMocks) SetupTest() {
	suite.client = &mocks.Client{}
}

type skuTestSuite struct {
	suite.Suite
	skuMocks

	skuService billing_grpc.SkuServiceServer
}

func TestSku(t *testing.T) {
	suite.Run(t, new(skuTestSuite))
}

func (suite *skuTestSuite) SetupTest() {
	suite.skuMocks.SetupTest()
	suite.skuService = NewSkuService(suite.skuMocks.client)
}

func mapConsoleSku(grpcResponse *billing_grpc.Sku) *console.Sku {
	return &console.Sku{
		ID:              grpcResponse.Id,
		Name:            grpcResponse.Name,
		Description:     grpcResponse.Description,
		ServiceID:       grpcResponse.ServiceId,
		PricingUnit:     grpcResponse.PricingUnit,
		PricingVersions: mapConsolePricingVersions(grpcResponse.PricingVersions),
	}
}

func mapConsolePricingExpressions(consoleResponse []*billing_grpc.PricingExpression) []console.PricingExpressionResponse {
	var expression []console.PricingExpressionResponse
	for _, consolePricingExpression := range consoleResponse {
		expression = append(expression, console.PricingExpressionResponse{
			Rates: mapConsoleRates(consolePricingExpression.Rates),
		})
	}
	return expression
}

func mapConsoleRates(gprcResponse []*billing_grpc.Rate) []console.RateResponse {
	var rates []console.RateResponse
	for _, gprcRate := range gprcResponse {
		rates = append(rates, console.RateResponse{
			StartPricingQuantity: gprcRate.StartPricingQuantity,
			UnitPrice:            gprcRate.UnitPrice,
			Currency:             gprcRate.Currency,
		})
	}
	return rates
}

func mapConsolePricingVersions(grpcResponse []*billing_grpc.PricingVersion) []console.PricingVersionResponse {
	var versions []console.PricingVersionResponse
	for _, grpcVersion := range grpcResponse {
		versions = append(versions, console.PricingVersionResponse{
			EffectiveTime:     grpcVersion.EffectiveTime.Seconds,
			PricingExpression: (mapConsolePricingExpressions(grpcVersion.PricingExpressions))[0],
			Type:              grpcVersion.Type.String(),
		})
	}
	return versions
}

func (suite *skuTestSuite) TestGetSkuError() {
	suite.client.On("GetSku", mock.Anything, mock.Anything).Return(nil, console.ErrServerInternal)

	req := &billing_grpc.GetSkuRequest{
		Id: generateID(),
	}
	_, err := suite.skuService.Get(getContextWithAuth(), req)

	suite.Require().Error(err)

	sts, ok := status.FromError(err)
	suite.Require().True(ok)
	suite.Require().Equal(sts.Code(), codes.Internal)
}

func (suite *skuTestSuite) TestGetSkuOK() {
	var rates []*billing_grpc.Rate
	rates = append(rates, &billing_grpc.Rate{
		StartPricingQuantity: "a",
		UnitPrice:            "q",
		Currency:             "RUB",
	})

	var expressions []*billing_grpc.PricingExpression
	expressions = append(expressions, &billing_grpc.PricingExpression{
		Rates: rates,
	})

	var versions []*billing_grpc.PricingVersion
	versions = append(versions, &billing_grpc.PricingVersion{
		EffectiveTime:      &timestamppb.Timestamp{Seconds: 2000000},
		PricingExpressions: expressions,
		Type:               billing_grpc.PricingVersionType_CONTRACT_PRICE,
	})

	expectedSku := &billing_grpc.Sku{
		Id:              "foo",
		Name:            "bar",
		Description:     "test",
		ServiceId:       "123",
		PricingUnit:     "q",
		PricingVersions: versions,
	}

	suite.client.On("GetSku", mock.Anything, mock.Anything).Return(mapConsoleSku(expectedSku), nil)

	req := &billing_grpc.GetSkuRequest{
		Id:               generateID(),
		Currency:         "RUB",
		BillingAccountId: generateID(),
	}
	service, err := suite.skuService.Get(getContextWithAuth(), req)

	suite.Require().NoError(err)
	suite.Require().Equal(service, expectedSku)
}
