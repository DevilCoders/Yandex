package mocks

import (
	"context"

	"github.com/stretchr/testify/mock"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/clients/nbs"
)

////////////////////////////////////////////////////////////////////////////////

type FactoryMock struct {
	mock.Mock
}

func (f *FactoryMock) HasClient(zoneID string) bool {
	args := f.Called(zoneID)
	return args.Bool(0)
}

func (f *FactoryMock) GetClient(
	ctx context.Context,
	zoneID string,
) (nbs.Client, error) {

	args := f.Called(ctx, zoneID)
	res, _ := args.Get(0).(nbs.Client)

	return res, args.Error(1)
}

func (f *FactoryMock) GetClientFromDefaultZone(
	ctx context.Context,
) (nbs.Client, error) {

	args := f.Called(ctx)
	res, _ := args.Get(0).(nbs.Client)

	return res, args.Error(1)
}

////////////////////////////////////////////////////////////////////////////////

func CreateFactoryMock() *FactoryMock {
	return &FactoryMock{}
}

////////////////////////////////////////////////////////////////////////////////

// Ensure that FactoryMock implements Factory.
func assertFactoryMockIsFactory(arg *FactoryMock) nbs.Factory {
	return arg
}
