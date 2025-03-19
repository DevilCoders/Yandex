package billing

import (
	"context"
	"fmt"
	"testing"

	"github.com/stretchr/testify/suite"

	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/adapters/mocks"
	"a.yandex-team.ru/cloud/marketplace/pkg/auth"
	"a.yandex-team.ru/cloud/marketplace/pkg/clients/billing-private"
)

func TestCreateSku(t *testing.T) {
	suite.Run(t, new(CreateSkuTestSuite))
}

type BaseBillingAdapterTestSuit struct {
	suite.Suite

	adapter BillingAdapter

	// NOTE: would be nice to have mocks for token acquisition.
	token string

	billingMock *mocks.Billing
}

func (suite *BaseBillingAdapterTestSuit) SetupTest() {
	suite.billingMock = &mocks.Billing{}
	suite.token = "secure-token"
	suite.adapter = NewBillingAdapter(suite.billingMock, auth.NewTokenAuthenticator(suite.token))
}

type CreateSkuTestSuite struct {
	BaseBillingAdapterTestSuit
}

func (suite *CreateSkuTestSuite) SetupTest() {
	suite.BaseBillingAdapterTestSuit.SetupTest()
}

func (suite *CreateSkuTestSuite) TestSuccess() {
	expected := billing.Sku{
		ID: "id",
	}

	params := billing.CreateSkuParams{}

	suite.billingMock.On("CreateSku", params).Return(expected, nil)
	result, err := suite.adapter.CreateSku(context.Background(), CreateSkuParams{})

	suite.Require().NoError(err)
	suite.Require().NotNil(result)

	suite.Require().Equal(expected.ID, result.ID)
}

func (suite *CreateSkuTestSuite) TestEmptyResponseFailure() {
	expectedErr := fmt.Errorf("404")
	params := billing.CreateSkuParams{}

	suite.billingMock.On("CreateSku", params).Return(billing.Sku{}, expectedErr)
	result, err := suite.adapter.CreateSku(context.Background(), CreateSkuParams{})

	suite.Require().Error(err)
	suite.Require().Nil(result)
}

func (suite *CreateSkuTestSuite) TestRequestFailure() {
	expectedErr := fmt.Errorf("request failed")
	params := billing.CreateSkuParams{}

	suite.billingMock.On("CreateSku", params).Return(billing.Sku{}, expectedErr)

	result, err := suite.adapter.CreateSku(context.Background(), CreateSkuParams{})

	suite.Require().Error(err)
	suite.Require().EqualError(expectedErr, err.Error())
	suite.Require().Nil(result)
}
