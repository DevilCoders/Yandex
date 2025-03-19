package services

import (
	"context"

	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/status"
	"google.golang.org/protobuf/types/known/timestamppb"

	"a.yandex-team.ru/cloud/billing/go/public_api/internal/console"
	billing_grpc "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/billing/v1"
)

var _ billing_grpc.BillingAccountServiceServer = &BillingAccountService{}

type BillingAccountService struct {
	client console.Client
}

func NewBillingAccountService(client console.Client) *BillingAccountService {
	return &BillingAccountService{client: client}
}

func (b BillingAccountService) Get(ctx context.Context, request *billing_grpc.GetBillingAccountRequest) (*billing_grpc.BillingAccount, error) {
	token, err := getToken(ctx)
	if err != nil {
		return nil, status.Error(codes.Unauthenticated, err.Error())
	}
	consoleReq := &console.GetBillingAccountRequest{
		AuthData:         console.AuthData{Token: token},
		BillingAccountID: request.Id,
	}

	account, err := b.client.GetBillingAccount(ctx, consoleReq)
	if err != nil {
		return nil, parseError(err)
	}

	grpcAccount := &billing_grpc.BillingAccount{
		Id:          account.ID,
		Name:        account.Name,
		CreatedAt:   &timestamppb.Timestamp{Seconds: account.CreatedAt},
		CountryCode: account.CountryCode,
		Currency:    account.Currency,
		Active:      account.Active,
		Balance:     account.Balance,
	}

	return grpcAccount, nil
}

func (b BillingAccountService) List(ctx context.Context, request *billing_grpc.ListBillingAccountsRequest) (*billing_grpc.ListBillingAccountsResponse, error) {
	token, err := getToken(ctx)
	if err != nil {
		return nil, status.Error(codes.Unauthenticated, err.Error())
	}

	consoleReq := &console.ListBillingAccountsRequest{
		AuthData: console.AuthData{Token: token},
	}

	if request.PageToken != "" {
		consoleReq.PageToken = &request.PageToken
	}

	if request.PageSize != 0 {
		consoleReq.PageSize = &request.PageSize
	}

	accounts, err := b.client.ListBillingAccounts(ctx, consoleReq)
	if err != nil {
		return nil, parseError(err)
	}

	var grpcAccounts []*billing_grpc.BillingAccount
	for _, account := range accounts.BillingAccounts {

		grpcAccounts = append(grpcAccounts, &billing_grpc.BillingAccount{
			Id:          account.ID,
			Name:        account.Name,
			CreatedAt:   &timestamppb.Timestamp{Seconds: account.CreatedAt},
			CountryCode: account.CountryCode,
			Currency:    account.Currency,
			Active:      account.Active,
			Balance:     account.Balance,
		})
	}

	response := &billing_grpc.ListBillingAccountsResponse{
		BillingAccounts: grpcAccounts,
	}

	if accounts.NextPageToken != nil {
		response.NextPageToken = *accounts.NextPageToken
	} else {
		response.NextPageToken = ""
	}

	return response, nil
}
