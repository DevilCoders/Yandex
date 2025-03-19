package services

import (
	"testing"

	"github.com/stretchr/testify/mock"
	"github.com/stretchr/testify/suite"
	"google.golang.org/protobuf/types/known/timestamppb"

	"a.yandex-team.ru/cloud/billing/go/public_api/internal/console"
	"a.yandex-team.ru/cloud/billing/go/public_api/internal/grpc_server/services/mocks"
	billing_grpc "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/billing/v1"
)

type billingAccountsMocks struct {
	client *mocks.Client
}

func (suite *billingAccountsMocks) SetupTest() {
	suite.client = &mocks.Client{}
}

type billingAccountsTestSuite struct {
	suite.Suite
	billingAccountsMocks

	billingAccountService billing_grpc.BillingAccountServiceServer
}

func TestBillingAccounts(t *testing.T) {
	suite.Run(t, new(billingAccountsTestSuite))
}

func (suite *billingAccountsTestSuite) SetupTest() {
	suite.billingAccountsMocks.SetupTest()
	suite.billingAccountService = NewBillingAccountService(suite.billingAccountsMocks.client)
}

func (suite *billingAccountsTestSuite) TestListAccountsFirstPage() {
	var (
		nextPageToken           = "next_page"
		billingAccountID1       = "account1"
		billingAccountID2       = "account2"
		name                    = "name"
		countryCode             = "RU"
		currency                = "RUB"
		balance1                = "100"
		balance2                = "-100"
		createdAt         int64 = 100
	)

	context := getContextWithAuth()
	expectedToken, err := getToken(context)
	suite.Require().NoError(err)

	consoleAccount1 := console.BillingAccountResponse{
		ID:          billingAccountID1,
		Name:        name,
		CreatedAt:   createdAt,
		CountryCode: countryCode,
		Currency:    currency,
		Active:      true,
		Balance:     balance1,
	}

	consoleAccount2 := console.BillingAccountResponse{
		ID:          billingAccountID2,
		Name:        name,
		CreatedAt:   createdAt,
		CountryCode: countryCode,
		Currency:    currency,
		Active:      false,
		Balance:     balance2,
	}

	suite.client.
		On("ListBillingAccounts",
			mock.Anything, // ctx
			&console.ListBillingAccountsRequest{ // request
				AuthData: console.AuthData{Token: expectedToken},
			},
		).
		Return(
			&console.ListBillingAccountsResponse{
				BillingAccounts: []console.BillingAccountResponse{consoleAccount1, consoleAccount2},
				NextPageToken:   &nextPageToken,
			}, nil)

	expectedGrpc := &billing_grpc.ListBillingAccountsResponse{
		NextPageToken: nextPageToken,
		BillingAccounts: []*billing_grpc.BillingAccount{
			{
				Id:          billingAccountID1,
				Name:        name,
				CreatedAt:   &timestamppb.Timestamp{Seconds: createdAt},
				CountryCode: countryCode,
				Currency:    currency,
				Active:      true,
				Balance:     balance1,
			}, {
				Id:          billingAccountID2,
				Name:        name,
				CreatedAt:   &timestamppb.Timestamp{Seconds: createdAt},
				CountryCode: countryCode,
				Currency:    currency,
				Active:      false,
				Balance:     balance2,
			},
		},
	}

	req := &billing_grpc.ListBillingAccountsRequest{}
	response, err := suite.billingAccountService.List(context, req)

	suite.Require().NoError(err)
	suite.Require().Equal(response, expectedGrpc)

}
func (suite *billingAccountsTestSuite) TestListsLastPage() {
	var (
		pageToken               = "page"
		billingAccountID1       = "account1"
		billingAccountID2       = "account2"
		name                    = "name"
		countryCode             = "RU"
		currency                = "RUB"
		balance1                = "100"
		balance2                = "-100"
		createdAt         int64 = 100
	)

	context := getContextWithAuth()
	expectedToken, err := getToken(context)
	suite.Require().NoError(err)

	consoleAccount1 := console.BillingAccountResponse{
		ID:          billingAccountID1,
		Name:        name,
		CreatedAt:   createdAt,
		CountryCode: countryCode,
		Currency:    currency,
		Active:      true,
		Balance:     balance1,
	}

	consoleAccount2 := console.BillingAccountResponse{
		ID:          billingAccountID2,
		Name:        name,
		CreatedAt:   createdAt,
		CountryCode: countryCode,
		Currency:    currency,
		Active:      false,
		Balance:     balance2,
	}

	suite.client.
		On("ListBillingAccounts",
			mock.Anything, // ctx
			&console.ListBillingAccountsRequest{ // request
				AuthData:  console.AuthData{Token: expectedToken},
				PageToken: &pageToken,
			},
		).
		Return(
			&console.ListBillingAccountsResponse{
				BillingAccounts: []console.BillingAccountResponse{consoleAccount1, consoleAccount2},
			}, nil)

	expectedGrpc := &billing_grpc.ListBillingAccountsResponse{
		BillingAccounts: []*billing_grpc.BillingAccount{
			{
				Id:          billingAccountID1,
				Name:        name,
				CreatedAt:   &timestamppb.Timestamp{Seconds: createdAt},
				CountryCode: countryCode,
				Currency:    currency,
				Active:      true,
				Balance:     balance1,
			}, {
				Id:          billingAccountID2,
				Name:        name,
				CreatedAt:   &timestamppb.Timestamp{Seconds: createdAt},
				CountryCode: countryCode,
				Currency:    currency,
				Active:      false,
				Balance:     balance2,
			},
		},
	}

	req := &billing_grpc.ListBillingAccountsRequest{PageToken: pageToken}
	response, err := suite.billingAccountService.List(context, req)

	suite.Require().NoError(err)
	suite.Require().Equal(response, expectedGrpc)

}

func (suite *billingAccountsTestSuite) TestListsIntermediatePageWithPageSize() {
	var (
		pageToken               = "page"
		nextPageToken           = "next_page"
		pageSize          int64 = 100
		billingAccountID1       = "account1"
		billingAccountID2       = "account2"
		name                    = "name"
		countryCode             = "RU"
		currency                = "RUB"
		balance1                = "100"
		balance2                = "-100"
		createdAt         int64 = 100
	)

	context := getContextWithAuth()
	expectedToken, err := getToken(context)
	suite.Require().NoError(err)

	consoleAccount1 := console.BillingAccountResponse{
		ID:          billingAccountID1,
		Name:        name,
		CreatedAt:   createdAt,
		CountryCode: countryCode,
		Currency:    currency,
		Active:      true,
		Balance:     balance1,
	}

	consoleAccount2 := console.BillingAccountResponse{
		ID:          billingAccountID2,
		Name:        name,
		CreatedAt:   createdAt,
		CountryCode: countryCode,
		Currency:    currency,
		Active:      false,
		Balance:     balance2,
	}

	suite.client.
		On("ListBillingAccounts",
			mock.Anything, // ctx
			&console.ListBillingAccountsRequest{ // request
				AuthData:  console.AuthData{Token: expectedToken},
				PageToken: &pageToken,
				PageSize:  &pageSize,
			},
		).
		Return(
			&console.ListBillingAccountsResponse{
				BillingAccounts: []console.BillingAccountResponse{consoleAccount1, consoleAccount2},
				NextPageToken:   &nextPageToken,
			}, nil)

	expectedGrpc := &billing_grpc.ListBillingAccountsResponse{
		BillingAccounts: []*billing_grpc.BillingAccount{
			{
				Id:          billingAccountID1,
				Name:        name,
				CreatedAt:   &timestamppb.Timestamp{Seconds: createdAt},
				CountryCode: countryCode,
				Currency:    currency,
				Active:      true,
				Balance:     balance1,
			}, {
				Id:          billingAccountID2,
				Name:        name,
				CreatedAt:   &timestamppb.Timestamp{Seconds: createdAt},
				CountryCode: countryCode,
				Currency:    currency,
				Active:      false,
				Balance:     balance2,
			},
		},
		NextPageToken: nextPageToken,
	}

	req := &billing_grpc.ListBillingAccountsRequest{PageSize: pageSize, PageToken: pageToken}
	response, err := suite.billingAccountService.List(context, req)

	suite.Require().NoError(err)
	suite.Require().Equal(response, expectedGrpc)

}

func (suite *billingAccountsTestSuite) TestGet() {
	var (
		accountID         = generateID()
		accountName       = "name"
		countryCode       = "RU"
		currency          = "RUB"
		balance           = "123.45"
		createdAt   int64 = 100
	)

	context := getContextWithAuth()
	expectedToken, err := getToken(context)
	suite.Require().NoError(err)

	consoleAccount := console.BillingAccountResponse{
		ID:          accountID,
		Name:        accountName,
		CreatedAt:   createdAt,
		CountryCode: countryCode,
		Currency:    currency,
		Active:      true,
		Balance:     balance,
	}

	expectedHTTPRequest := &console.GetBillingAccountRequest{
		BillingAccountID: accountID,
		AuthData:         console.AuthData{Token: expectedToken},
	}

	suite.client.On("GetBillingAccount", mock.Anything, expectedHTTPRequest).Return(&consoleAccount, nil)

	expectedGrpcAccount := &billing_grpc.BillingAccount{
		Id:          accountID,
		Name:        accountName,
		CreatedAt:   &timestamppb.Timestamp{Seconds: createdAt},
		CountryCode: countryCode,
		Currency:    currency,
		Active:      true,
		Balance:     balance,
	}

	grpcRequest := &billing_grpc.GetBillingAccountRequest{Id: accountID}
	result, err := suite.billingAccountService.Get(context, grpcRequest)

	suite.Require().NoError(err)
	suite.Require().Equal(result, expectedGrpcAccount)
}
