package mocks

import (
	"context"

	"github.com/stretchr/testify/mock"

	"a.yandex-team.ru/cloud/marketplace/pkg/clients/billing-private"
)

type Billing struct {
	mock.Mock
}

func (b *Billing) SessionWithYCSubjectToken(ctx context.Context, iamToken string) billing.APISession {
	return b
}

func (b *Billing) SessionWithAuthToken(ctx context.Context, token string) billing.APISession {
	return b
}

func (b *Billing) CreateSku(params billing.CreateSkuParams) (billing.Sku, error) {
	args := b.Called(params)
	return args.Get(0).(billing.Sku), args.Error(1)
}

func (b *Billing) ResolveBillingAccounts(params billing.ResolveBillingAccountsParams) ([]billing.BillingAccount, error) {
	args := b.Called(params)
	return args.Get(0).([]billing.BillingAccount), args.Error(1)
}

func (b *Billing) ResolveBillingAccountByCloudIDFull(cloudID string, _ int64) (*billing.ExtendedBillingAccountCamelCaseView, error) {
	args := b.Called(cloudID)
	return args.Get(0).(*billing.ExtendedBillingAccountCamelCaseView), args.Error(1)
}
