package mocks

import (
	"context"

	"github.com/stretchr/testify/mock"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/transfer"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/transfer/protos"
)

////////////////////////////////////////////////////////////////////////////////

type ServiceMock struct {
	mock.Mock
}

func (s *ServiceMock) TransferFromImageToDisk(
	ctx context.Context,
	req *protos.TransferFromImageToDiskRequest,
) (string, error) {

	args := s.Called(ctx, req)
	return args.String(0), args.Error(1)
}

func (s *ServiceMock) TransferFromSnapshotToDisk(
	ctx context.Context,
	req *protos.TransferFromSnapshotToDiskRequest,
) (string, error) {

	args := s.Called(ctx, req)
	return args.String(0), args.Error(1)
}

////////////////////////////////////////////////////////////////////////////////

func CreateServiceMock() *ServiceMock {
	return &ServiceMock{}
}

////////////////////////////////////////////////////////////////////////////////

// Ensure that ServiceMock implements Service.
func assertServiceMockIsService(arg *ServiceMock) transfer.Service {
	return arg
}
