package mocks

import (
	"context"

	"github.com/stretchr/testify/mock"

	"a.yandex-team.ru/cloud/marketplace/pkg/clients/marketplace-private"
)

type Marketplace struct {
	mock.Mock
}

func (m *Marketplace) GetTariff(params marketplace.GetTariffParams) (*marketplace.Tariff, error) {
	args := m.Called(params)
	return args.Get(0).(*marketplace.Tariff), args.Error(1)
}

func (m *Marketplace) GetPublisherByID(publisherID string) (*marketplace.Publisher, error) {
	args := m.Called(publisherID)
	return args.Get(0).(*marketplace.Publisher), args.Error(1)
}

func (m *Marketplace) GetCategoryByIDorName(categoryID string) (*marketplace.Category, error) {
	args := m.Called(categoryID)
	return args.Get(0).(*marketplace.Category), args.Error(1)
}

func (m *Marketplace) GetProductByID(params marketplace.GetProductParams) (*marketplace.Product, error) {
	args := m.Called(params)
	return args.Get(0).(*marketplace.Product), args.Error(1)
}

func (m *Marketplace) GetVersionByID(params marketplace.GetVersionParams) (*marketplace.Version, error) {
	args := m.Called(params)
	return args.Get(0).(*marketplace.Version), args.Error(1)
}

func (m *Marketplace) SyncProduct(params marketplace.SyncProductParams) (*marketplace.Operation, error) {
	args := m.Called(params)
	return args.Get(0).(*marketplace.Operation), args.Error(1)
}

func (m *Marketplace) SyncVersion(params marketplace.SyncVersionParams) (*marketplace.Operation, error) {
	args := m.Called(params)
	return args.Get(0).(*marketplace.Operation), args.Error(1)
}

func (m *Marketplace) ListTariffsByProductID(publisherID, productID string) ([]marketplace.Tariff, error) {
	args := m.Called(publisherID, productID)
	return args.Get(0).([]marketplace.Tariff), args.Error(1)
}

func (m *Marketplace) CreateTariff(params marketplace.CreateTariffParams) (*marketplace.TariffCreateOperation, error) {
	args := m.Called(params)
	return args.Get(0).(*marketplace.TariffCreateOperation), args.Error(1)
}

func (m *Marketplace) SessionWithYCSubjectToken(ctx context.Context, iamToken string) marketplace.APISession {
	return m
}

func (m *Marketplace) SessionWithAuthToken(ctx context.Context, token string) marketplace.APISession {
	return m
}

func (m *Marketplace) GetOperationByID(operationID string) (*marketplace.Operation, error) {
	args := m.Called(operationID)
	return args.Get(0).(*marketplace.Operation), args.Error(1)
}

func (m *Marketplace) WaitOperation(operationID string) (*marketplace.Operation, error) {
	args := m.Called(operationID)
	return args.Get(0).(*marketplace.Operation), args.Error(1)
}
