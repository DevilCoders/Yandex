package storage

import (
	"context"
	"time"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/types"
)

////////////////////////////////////////////////////////////////////////////////

type BaseDisk struct {
	// Required params, needed to locate base disk in DB.
	ID      string
	ImageID string
	ZoneID  string

	// Optional params.
	SrcDisk             *types.Disk
	SrcDiskCheckpointID string
	CheckpointID        string
	CreateTaskID        string
	Size                uint64
	Ready               bool
}

type Slot struct {
	OverlayDisk     *types.Disk
	OverlayDiskKind types.DiskKind
	OverlayDiskSize uint64
}

type RebaseInfo struct {
	OverlayDisk      *types.Disk
	BaseDiskID       string
	TargetBaseDiskID string
	SlotGeneration   uint64
}

type PoolInfo struct {
	ImageID       string
	ZoneID        string
	FreeUnits     uint64
	AcquiredUnits uint64
	Capacity      uint32
	ImageSize     uint64
	CreatedAt     time.Time
}

type Storage interface {
	// Acquires slot for given |slot.OverlayDisk|.
	AcquireBaseDiskSlot(
		ctx context.Context,
		imageID string,
		slot Slot,
	) (BaseDisk, error)

	// Releases slot for given |overlayDisk|.
	// If all slots are released && base disk does not belong to any alive pool
	// => base disk will be deleted.
	ReleaseBaseDiskSlot(
		ctx context.Context,
		overlayDisk *types.Disk,
	) (baseDisk BaseDisk, err error)

	OverlayDiskRebasing(ctx context.Context, info RebaseInfo) error

	OverlayDiskRebased(ctx context.Context, info RebaseInfo) error

	BaseDiskCreated(ctx context.Context, baseDisk BaseDisk) error

	BaseDiskCreationFailed(ctx context.Context, baseDisk BaseDisk) error

	// Returns base disks to schedule creation for.
	TakeBaseDisksToSchedule(ctx context.Context) ([]BaseDisk, error)

	BaseDisksScheduled(ctx context.Context, baseDisks []BaseDisk) error

	IsPoolConfigured(
		ctx context.Context,
		imageID string,
		zoneID string,
	) (bool, error)

	ConfigurePool(
		ctx context.Context,
		imageID string,
		zoneID string,
		capacity uint32,
		imageSize uint64,
	) error

	// Deletes pools of all kinds (ssd, hdd, ...) matching given |imageID|, |zoneID|.
	DeletePool(ctx context.Context, imageID string, zoneID string) error

	ImageDeleting(ctx context.Context, imageID string) error

	GetBaseDisksToDelete(ctx context.Context, limit uint64) ([]BaseDisk, error)

	BaseDisksDeleted(ctx context.Context, baseDisks []BaseDisk) error

	ClearDeletedBaseDisks(
		ctx context.Context,
		deletedBefore time.Time,
		limit int,
	) error

	ClearReleasedSlots(
		ctx context.Context,
		releasedBefore time.Time,
		limit int,
	) error

	RetireBaseDisk(
		ctx context.Context,
		baseDiskID string,
		srcDisk *types.Disk,
		srcDiskCheckpointID string,
		srcDiskCheckpointSize uint64,
	) ([]RebaseInfo, error)

	IsBaseDiskRetired(ctx context.Context, baseDiskID string) (bool, error)

	ListBaseDisks(
		ctx context.Context,
		imageID string,
		zoneID string,
	) ([]BaseDisk, error)

	// Locks pool deletion.
	LockPool(
		ctx context.Context,
		imageID string,
		zoneID string,
		lockID string,
	) (bool, error)

	UnlockPool(
		ctx context.Context,
		imageID string,
		zoneID string,
		lockID string,
	) error

	GetReadyPoolInfos(ctx context.Context) ([]PoolInfo, error)
}
