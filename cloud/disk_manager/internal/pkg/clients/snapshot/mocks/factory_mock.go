package mocks

import (
	"context"

	"github.com/stretchr/testify/mock"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/clients/snapshot"
)

////////////////////////////////////////////////////////////////////////////////

type FactoryMock struct {
	mock.Mock
}

func (f *FactoryMock) CreateClient(ctx context.Context) (snapshot.Client, error) {
	args := f.Called(ctx)
	res, _ := args.Get(0).(snapshot.Client)

	return res, args.Error(1)
}

func (f *FactoryMock) CreateClientFromZone(
	ctx context.Context,
	zoneID string,
) (snapshot.Client, error) {

	args := f.Called(ctx, zoneID)
	res, _ := args.Get(0).(snapshot.Client)
	return res, args.Error(1)
}

////////////////////////////////////////////////////////////////////////////////

func CreateFactoryMock() *FactoryMock {
	return &FactoryMock{}
}

////////////////////////////////////////////////////////////////////////////////

// Ensure that FactoryMock implements Factory.
func assertFactoryMockIsFactory(arg *FactoryMock) snapshot.Factory {
	return arg
}
