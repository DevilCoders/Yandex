package adapters

import (
	"context"
	"fmt"
	"testing"

	"github.com/stretchr/testify/suite"

	"a.yandex-team.ru/cloud/marketplace/lich/internal/core/adapters/mocks"
	"a.yandex-team.ru/cloud/marketplace/pkg/auth"
	"a.yandex-team.ru/cloud/marketplace/pkg/clients/billing-private"
)

const (
	testCloudID = "cloud-id"
)

func TestGetBillingAccountByCloudID(t *testing.T) {
	suite.Run(t, new(GetBillingAccountByCloudIDTestSuite))
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

type GetBillingAccountByCloudIDTestSuite struct {
	BaseBillingAdapterTestSuit

	cloudID string
}

func (suite *GetBillingAccountByCloudIDTestSuite) SetupTest() {
	suite.BaseBillingAdapterTestSuit.SetupTest()

	suite.cloudID = testCloudID

}

func (suite *GetBillingAccountByCloudIDTestSuite) TestSuccess() {
	cloudID := "test.cloud.id"

	expectedBillingAccount := billing.BillingAccount{ID: "id1", Name: "boo", UsageStatus: "     no-status"}
	expected := []billing.BillingAccount{
		expectedBillingAccount,
		{ID: "id2", Name: "foo", UsageStatus: "   some-status"},
		{ID: "id3", Name: "bar", UsageStatus: "billing-status"},
	}

	params := billing.ResolveBillingAccountsParams{
		CloudID: cloudID,
	}

	suite.billingMock.On("ResolveBillingAccounts", params).Return(expected, nil)
	result, err := suite.adapter.GetBillingAccountByCloudID(context.Background(), cloudID)

	suite.Require().NoError(err)
	suite.Require().NotNil(result)

	suite.Require().Equal(expectedBillingAccount.ID, result.ID)
	suite.Require().Equal(expectedBillingAccount.Name, result.Name)
	suite.Require().Equal(expectedBillingAccount.UsageStatus, result.UsageStatus)
}

func (suite *GetBillingAccountByCloudIDTestSuite) TestEmptyResponseFailure() {
	cloudID := "test.cloud.id"

	expected := []billing.BillingAccount{}

	params := billing.ResolveBillingAccountsParams{
		CloudID: cloudID,
	}

	suite.billingMock.On("ResolveBillingAccounts", params).Return(expected, nil)
	result, err := suite.adapter.GetBillingAccountByCloudID(context.Background(), cloudID)

	suite.Require().Error(err)
	suite.Require().Nil(result)
}

func (suite *GetBillingAccountByCloudIDTestSuite) TestRequestFailure() {
	cloudID := "test.cloud.id"

	expectedErr := fmt.Errorf("request failed")
	params := billing.ResolveBillingAccountsParams{
		CloudID: cloudID,
	}

	suite.billingMock.
		On("ResolveBillingAccounts", params).Return([]billing.BillingAccount(nil), expectedErr)

	result, err := suite.adapter.GetBillingAccountByCloudID(context.Background(), cloudID)

	suite.Require().Error(err)
	suite.Require().EqualError(expectedErr, err.Error())
	suite.Require().Nil(result)
}
