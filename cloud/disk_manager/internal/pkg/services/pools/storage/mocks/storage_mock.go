package mocks

import (
	"context"
	"time"

	"github.com/stretchr/testify/mock"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/pools/storage"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/types"
)

////////////////////////////////////////////////////////////////////////////////

type StorageMock struct {
	mock.Mock
}

func (s *StorageMock) AcquireBaseDiskSlot(
	ctx context.Context,
	imageID string,
	slot storage.Slot,
) (storage.BaseDisk, error) {

	args := s.Called(ctx, imageID, slot)
	return args.Get(0).(storage.BaseDisk), args.Error(1)
}

func (s *StorageMock) ReleaseBaseDiskSlot(
	ctx context.Context,
	overlayDisk *types.Disk,
) (storage.BaseDisk, error) {

	args := s.Called(ctx, overlayDisk)
	return args.Get(0).(storage.BaseDisk), args.Error(1)
}

func (s *StorageMock) OverlayDiskRebasing(
	ctx context.Context,
	info storage.RebaseInfo,
) error {

	args := s.Called(ctx, info)
	return args.Error(0)
}

func (s *StorageMock) OverlayDiskRebased(
	ctx context.Context,
	info storage.RebaseInfo,
) error {

	args := s.Called(ctx, info)
	return args.Error(0)
}

func (s *StorageMock) BaseDiskCreated(
	ctx context.Context,
	baseDisk storage.BaseDisk,
) error {

	args := s.Called(ctx, baseDisk)
	return args.Error(0)
}

func (s *StorageMock) BaseDiskCreationFailed(
	ctx context.Context,
	baseDisk storage.BaseDisk,
) error {

	args := s.Called(ctx, baseDisk)
	return args.Error(0)
}

func (s *StorageMock) TakeBaseDisksToSchedule(
	ctx context.Context,
) ([]storage.BaseDisk, error) {

	args := s.Called(ctx)
	return args.Get(0).([]storage.BaseDisk), args.Error(1)
}

func (s *StorageMock) BaseDisksScheduled(
	ctx context.Context,
	baseDisks []storage.BaseDisk,
) error {

	args := s.Called(ctx, baseDisks)
	return args.Error(0)
}

func (s *StorageMock) ConfigurePool(
	ctx context.Context,
	imageID string,
	zoneID string,
	capacity uint32,
	imageSize uint64,
) error {

	args := s.Called(ctx, imageID, zoneID, capacity, imageSize)
	return args.Error(0)
}

func (s *StorageMock) IsPoolConfigured(
	ctx context.Context,
	imageID string,
	zoneID string,
) (bool, error) {

	args := s.Called(ctx, imageID, zoneID)
	return args.Bool(0), args.Error(1)
}

func (s *StorageMock) DeletePool(
	ctx context.Context,
	imageID string,
	zoneID string,
) error {

	args := s.Called(ctx, imageID, zoneID)
	return args.Error(0)
}

func (s *StorageMock) ImageDeleting(
	ctx context.Context,
	imageID string,
) error {

	args := s.Called(ctx, imageID)
	return args.Error(0)
}

func (s *StorageMock) GetBaseDisksToDelete(
	ctx context.Context,
	limit uint64,
) ([]storage.BaseDisk, error) {

	args := s.Called(ctx, limit)
	return args.Get(0).([]storage.BaseDisk), args.Error(1)
}

func (s *StorageMock) BaseDisksDeleted(
	ctx context.Context,
	baseDisks []storage.BaseDisk,
) error {

	args := s.Called(ctx, baseDisks)
	return args.Error(0)
}

func (s *StorageMock) ClearDeletedBaseDisks(
	ctx context.Context,
	deletedBefore time.Time,
	limit int,
) error {

	args := s.Called(ctx, deletedBefore, limit)
	return args.Error(0)
}

func (s *StorageMock) ClearReleasedSlots(
	ctx context.Context,
	releasedBefore time.Time,
	limit int,
) error {

	args := s.Called(ctx, releasedBefore, limit)
	return args.Error(0)
}

func (s *StorageMock) RetireBaseDisk(
	ctx context.Context,
	baseDiskID string,
	srcDisk *types.Disk,
	srcDiskCheckpointID string,
	srcDiskCheckpointSize uint64,
) ([]storage.RebaseInfo, error) {

	args := s.Called(
		ctx,
		baseDiskID,
		srcDisk,
		srcDiskCheckpointID,
		srcDiskCheckpointSize,
	)
	return args.Get(0).([]storage.RebaseInfo), args.Error(1)
}

func (s *StorageMock) IsBaseDiskRetired(
	ctx context.Context,
	baseDiskID string,
) (bool, error) {

	args := s.Called(ctx, baseDiskID)
	return args.Bool(0), args.Error(1)
}

func (s *StorageMock) ListBaseDisks(
	ctx context.Context,
	imageID string,
	zoneID string,
) ([]storage.BaseDisk, error) {

	args := s.Called(ctx, imageID, zoneID)
	return args.Get(0).([]storage.BaseDisk), args.Error(0)
}

func (s *StorageMock) LockPool(
	ctx context.Context,
	imageID string,
	zoneID string,
	lockID string,
) (bool, error) {

	args := s.Called(ctx, imageID, zoneID, lockID)
	return args.Bool(0), args.Error(1)
}

func (s *StorageMock) UnlockPool(
	ctx context.Context,
	imageID string,
	zoneID string,
	lockID string,
) error {

	args := s.Called(ctx, imageID, zoneID, lockID)
	return args.Error(0)
}

func (s *StorageMock) GetReadyPoolInfos(
	ctx context.Context,
) ([]storage.PoolInfo, error) {

	args := s.Called(ctx)
	return args.Get(0).([]storage.PoolInfo), args.Error(1)
}

////////////////////////////////////////////////////////////////////////////////

func CreateStorageMock() *StorageMock {
	return &StorageMock{}
}

////////////////////////////////////////////////////////////////////////////////

// Ensure that StorageMock implements Storage.
func assertStorageMockIsStorage(arg *StorageMock) storage.Storage {
	return arg
}
