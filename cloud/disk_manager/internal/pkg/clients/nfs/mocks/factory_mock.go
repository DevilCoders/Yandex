package mocks

import (
	"context"

	"github.com/stretchr/testify/mock"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/clients/nfs"
)

////////////////////////////////////////////////////////////////////////////////

type FactoryMock struct {
	mock.Mock
}

func (f *FactoryMock) CreateClient(
	ctx context.Context,
	zoneID string,
) (nfs.Client, error) {

	args := f.Called(ctx, zoneID)
	res, _ := args.Get(0).(nfs.Client)

	return res, args.Error(1)
}

func (f *FactoryMock) CreateClientFromDefaultZone(
	ctx context.Context,
) (nfs.Client, error) {

	args := f.Called(ctx)
	res, _ := args.Get(0).(nfs.Client)

	return res, args.Error(1)
}

////////////////////////////////////////////////////////////////////////////////

func CreateFactoryMock() *FactoryMock {
	return &FactoryMock{}
}

////////////////////////////////////////////////////////////////////////////////

// Ensure that FactoryMock implements Factory.
func assertFactoryMockIsFactory(arg *FactoryMock) nfs.Factory {
	return arg
}
