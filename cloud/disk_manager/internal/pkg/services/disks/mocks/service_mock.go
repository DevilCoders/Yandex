package mocks

import (
	"context"

	"github.com/stretchr/testify/mock"

	"a.yandex-team.ru/cloud/disk_manager/api/yandex/cloud/priv/disk_manager/v1"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/disks"
)

////////////////////////////////////////////////////////////////////////////////

type ServiceMock struct {
	mock.Mock
}

func (s *ServiceMock) CreateDisk(
	ctx context.Context,
	req *disk_manager.CreateDiskRequest,
) (string, error) {

	args := s.Called(ctx, req)
	return args.String(0), args.Error(1)
}

func (s *ServiceMock) DeleteDisk(
	ctx context.Context,
	req *disk_manager.DeleteDiskRequest,
) (string, error) {

	args := s.Called(ctx, req)
	return args.String(0), args.Error(1)
}

func (s *ServiceMock) ResizeDisk(
	ctx context.Context,
	req *disk_manager.ResizeDiskRequest,
) (string, error) {

	args := s.Called(ctx, req)
	return args.String(0), args.Error(1)
}

func (s *ServiceMock) AlterDisk(
	ctx context.Context,
	req *disk_manager.AlterDiskRequest,
) (string, error) {

	args := s.Called(ctx, req)
	return args.String(0), args.Error(1)
}

func (s *ServiceMock) AssignDisk(
	ctx context.Context,
	req *disk_manager.AssignDiskRequest,
) (string, error) {

	args := s.Called(ctx, req)
	return args.String(0), args.Error(1)
}

func (s *ServiceMock) UnassignDisk(
	ctx context.Context,
	req *disk_manager.UnassignDiskRequest,
) (string, error) {

	args := s.Called(ctx, req)
	return args.String(0), args.Error(1)
}

func (s *ServiceMock) DescribeDiskModel(
	ctx context.Context,
	req *disk_manager.DescribeDiskModelRequest,
) (*disk_manager.DiskModel, error) {

	args := s.Called(ctx, req)
	return args.Get(0).(*disk_manager.DiskModel), args.Error(1)
}

func (s *ServiceMock) StatDisk(
	ctx context.Context,
	req *disk_manager.StatDiskRequest,
) (*disk_manager.DiskStats, error) {

	args := s.Called(ctx, req)
	return args.Get(0).(*disk_manager.DiskStats), args.Error(1)
}

func (s *ServiceMock) MigrateDisk(
	ctx context.Context,
	req *disk_manager.MigrateDiskRequest,
) (string, error) {

	args := s.Called(ctx, req)
	return args.String(0), args.Error(1)
}

func (s *ServiceMock) SendMigrationSignal(
	ctx context.Context,
	req *disk_manager.SendMigrationSignalRequest,
) error {

	args := s.Called(ctx, req)
	return args.Error(0)
}

////////////////////////////////////////////////////////////////////////////////

func CreateServiceMock() *ServiceMock {
	return &ServiceMock{}
}

////////////////////////////////////////////////////////////////////////////////

// Ensure that ServiceMock implements Service.
func assertServiceMockIsService(arg *ServiceMock) disks.Service {
	return arg
}
