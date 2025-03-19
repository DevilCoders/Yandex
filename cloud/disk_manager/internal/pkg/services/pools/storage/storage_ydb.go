package storage

import (
	"context"
	"fmt"
	"time"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/logging"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/persistence"
	pools_config "a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/pools/config"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/types"
)

////////////////////////////////////////////////////////////////////////////////

type storageYDB struct {
	db                                 *persistence.YDBClient
	tablesPath                         string
	maxActiveSlots                     uint64
	maxBaseDisksInflight               uint64
	maxBaseDiskUnits                   uint64
	takeBaseDisksToScheduleParallelism int
}

////////////////////////////////////////////////////////////////////////////////

func (s *storageYDB) AcquireBaseDiskSlot(
	ctx context.Context,
	imageID string,
	slot Slot,
) (BaseDisk, error) {

	var baseDisk BaseDisk

	err := s.db.Execute(
		ctx,
		func(ctx context.Context, session *persistence.Session) error {
			var err error
			baseDisk, err = s.acquireBaseDiskSlot(
				ctx,
				session,
				imageID,
				slot,
			)
			return err
		},
	)
	if err != nil {
		logging.Warn(ctx, "AcquireBaseDiskSlot failed: %v", err)
	}

	return baseDisk, err
}

func (s *storageYDB) ReleaseBaseDiskSlot(
	ctx context.Context,
	overlayDisk *types.Disk,
) (BaseDisk, error) {

	var baseDisk BaseDisk

	err := s.db.Execute(
		ctx,
		func(ctx context.Context, session *persistence.Session) error {
			var err error
			baseDisk, err = s.releaseBaseDiskSlot(
				ctx,
				session,
				overlayDisk,
			)
			return err
		},
	)
	if err != nil {
		logging.Warn(ctx, "ReleaseBaseDiskSlot failed: %v", err)
	}

	return baseDisk, err
}

func (s *storageYDB) OverlayDiskRebasing(
	ctx context.Context,
	info RebaseInfo,
) error {

	err := s.db.Execute(
		ctx,
		func(ctx context.Context, session *persistence.Session) error {
			return s.overlayDiskRebasing(
				ctx,
				session,
				info,
			)
		},
	)
	if err != nil {
		logging.Warn(ctx, "OverlayDiskRebasing failed: %v", err)
	}

	return err
}

func (s *storageYDB) OverlayDiskRebased(
	ctx context.Context,
	info RebaseInfo,
) error {

	err := s.db.Execute(
		ctx,
		func(ctx context.Context, session *persistence.Session) error {
			return s.overlayDiskRebased(
				ctx,
				session,
				info,
			)
		},
	)
	if err != nil {
		logging.Warn(ctx, "OverlayDiskRebased failed: %v", err)
	}

	return err
}

func (s *storageYDB) BaseDiskCreated(
	ctx context.Context,
	baseDisk BaseDisk,
) error {

	err := s.db.Execute(
		ctx,
		func(ctx context.Context, session *persistence.Session) error {
			return s.baseDiskCreated(ctx, session, baseDisk)
		},
	)
	if err != nil {
		logging.Warn(ctx, "BaseDiskCreated failed: %v", err)
	}

	return err
}

func (s *storageYDB) BaseDiskCreationFailed(
	ctx context.Context,
	baseDisk BaseDisk,
) error {

	err := s.db.Execute(
		ctx,
		func(ctx context.Context, session *persistence.Session) error {
			return s.baseDiskCreationFailed(ctx, session, baseDisk)
		},
	)
	if err != nil {
		logging.Warn(ctx, "BaseDiskCreationFailed failed: %v", err)
	}

	return err
}

func (s *storageYDB) TakeBaseDisksToSchedule(
	ctx context.Context,
) ([]BaseDisk, error) {

	var baseDisks []BaseDisk

	err := s.db.Execute(
		ctx,
		func(ctx context.Context, session *persistence.Session) error {
			var err error
			baseDisks, err = s.takeBaseDisksToSchedule(ctx, session)
			return err
		},
	)
	if err != nil {
		logging.Warn(ctx, "TakeBaseDisksToSchedule failed: %v", err)
	}

	return baseDisks, err
}

func (s *storageYDB) GetReadyPoolInfos(
	ctx context.Context,
) ([]PoolInfo, error) {

	var poolInfos []PoolInfo

	err := s.db.Execute(
		ctx,
		func(ctx context.Context, session *persistence.Session) error {
			var err error
			poolInfos, err = s.getReadyPoolInfos(ctx, session)
			return err
		},
	)
	if err != nil {
		logging.Warn(ctx, "GetReadyPoolInfos failed: %v", err)
	}

	return poolInfos, err
}

func (s *storageYDB) BaseDisksScheduled(
	ctx context.Context,
	baseDisks []BaseDisk,
) error {

	err := s.db.Execute(
		ctx,
		func(ctx context.Context, session *persistence.Session) error {
			return s.baseDisksScheduled(ctx, session, baseDisks)
		},
	)
	if err != nil {
		logging.Warn(ctx, "BaseDisksScheduled failed: %v", err)
	}

	return err
}

func (s *storageYDB) ConfigurePool(
	ctx context.Context,
	imageID string,
	zoneID string,
	capacity uint32,
	imageSize uint64,
) error {

	err := s.db.Execute(
		ctx,
		func(ctx context.Context, session *persistence.Session) error {
			return s.configurePool(
				ctx,
				session,
				imageID,
				zoneID,
				capacity,
				imageSize,
			)
		},
	)
	if err != nil {
		logging.Warn(ctx, "ConfigurePool failed: %v", err)
	}

	return err
}

func (s *storageYDB) IsPoolConfigured(
	ctx context.Context,
	imageID string,
	zoneID string,
) (bool, error) {

	var res bool

	err := s.db.Execute(
		ctx,
		func(ctx context.Context, session *persistence.Session) error {
			var err error
			res, err = s.isPoolConfigured(ctx, session, imageID, zoneID)
			return err
		},
	)
	if err != nil {
		logging.Warn(ctx, "IsPoolConfigured failed: %v", err)
	}

	return res, err
}

func (s *storageYDB) DeletePool(
	ctx context.Context,
	imageID string,
	zoneID string,
) error {

	err := s.db.Execute(
		ctx,
		func(ctx context.Context, session *persistence.Session) error {
			return s.deletePool(ctx, session, imageID, zoneID)
		},
	)
	if err != nil {
		logging.Warn(ctx, "DeletePool failed: %v", err)
	}

	return err
}

func (s *storageYDB) ImageDeleting(
	ctx context.Context,
	imageID string,
) error {

	err := s.db.Execute(
		ctx,
		func(ctx context.Context, session *persistence.Session) error {
			return s.imageDeleting(ctx, session, imageID)
		},
	)
	if err != nil {
		logging.Warn(ctx, "ImageDeleting failed: %v", err)
	}

	return err
}

func (s *storageYDB) GetBaseDisksToDelete(
	ctx context.Context,
	limit uint64,
) ([]BaseDisk, error) {

	var baseDisks []BaseDisk

	err := s.db.Execute(
		ctx,
		func(ctx context.Context, session *persistence.Session) error {
			var err error
			baseDisks, err = s.getBaseDisksToDelete(ctx, session, limit)
			return err
		},
	)
	if err != nil {
		logging.Warn(ctx, "GetBaseDisksToDelete failed: %v", err)
	}

	return baseDisks, err
}

func (s *storageYDB) BaseDisksDeleted(
	ctx context.Context,
	baseDisks []BaseDisk,
) error {

	err := s.db.Execute(
		ctx,
		func(ctx context.Context, session *persistence.Session) error {
			return s.baseDisksDeleted(ctx, session, baseDisks)
		},
	)
	if err != nil {
		logging.Warn(ctx, "BaseDisksDeleted failed: %v", err)
	}

	return err
}
func (s *storageYDB) ClearDeletedBaseDisks(
	ctx context.Context,
	deletedBefore time.Time,
	limit int,
) error {

	err := s.db.Execute(
		ctx,
		func(ctx context.Context, session *persistence.Session) error {
			return s.clearDeletedBaseDisks(ctx, session, deletedBefore, limit)
		},
	)
	if err != nil {
		logging.Warn(ctx, "ClearDeletedBaseDisks failed: %v", err)
	}

	return err
}

func (s *storageYDB) ClearReleasedSlots(
	ctx context.Context,
	releasedBefore time.Time,
	limit int,
) error {

	err := s.db.Execute(
		ctx,
		func(ctx context.Context, session *persistence.Session) error {
			return s.clearReleasedSlots(ctx, session, releasedBefore, limit)
		},
	)
	if err != nil {
		logging.Warn(ctx, "ClearReleasedSlots failed: %v", err)
	}

	return err
}

func (s *storageYDB) RetireBaseDisk(
	ctx context.Context,
	baseDiskID string,
	srcDisk *types.Disk,
	srcDiskCheckpointID string,
	srcDiskCheckpointSize uint64,
) ([]RebaseInfo, error) {

	var rebaseInfos []RebaseInfo

	err := s.db.Execute(
		ctx,
		func(ctx context.Context, session *persistence.Session) error {
			var err error
			rebaseInfos, err = s.retireBaseDisk(
				ctx,
				session,
				baseDiskID,
				srcDisk,
				srcDiskCheckpointID,
				srcDiskCheckpointSize,
			)
			return err
		},
	)
	if err != nil {
		logging.Warn(ctx, "RetireBaseDisk failed: %v", err)
	}

	return rebaseInfos, err
}

func (s *storageYDB) IsBaseDiskRetired(
	ctx context.Context,
	baseDiskID string,
) (bool, error) {

	var retired bool

	err := s.db.Execute(
		ctx,
		func(ctx context.Context, session *persistence.Session) error {
			var err error
			retired, err = s.isBaseDiskRetired(ctx, session, baseDiskID)
			return err
		},
	)
	if err != nil {
		logging.Warn(ctx, "IsBaseDiskRetired failed: %v", err)
	}

	return retired, err
}

func (s *storageYDB) ListBaseDisks(
	ctx context.Context,
	imageID string,
	zoneID string,
) ([]BaseDisk, error) {

	var baseDisks []BaseDisk

	err := s.db.Execute(
		ctx,
		func(ctx context.Context, session *persistence.Session) error {
			var err error
			baseDisks, err = s.listBaseDisks(ctx, session, imageID, zoneID)
			return err
		},
	)
	if err != nil {
		logging.Warn(ctx, "ListBaseDisks failed: %v", err)
	}

	return baseDisks, err
}

func (s *storageYDB) LockPool(
	ctx context.Context,
	imageID string,
	zoneID string,
	lockID string,
) (bool, error) {

	var locked bool

	err := s.db.Execute(
		ctx,
		func(ctx context.Context, session *persistence.Session) error {
			var err error
			locked, err = s.lockPool(ctx, session, imageID, zoneID, lockID)
			return err
		},
	)
	if err != nil {
		logging.Warn(ctx, "LockPool failed: %v", err)
	}

	return locked, err
}

func (s *storageYDB) UnlockPool(
	ctx context.Context,
	imageID string,
	zoneID string,
	lockID string,
) error {

	err := s.db.Execute(
		ctx,
		func(ctx context.Context, session *persistence.Session) error {
			return s.unlockPool(ctx, session, imageID, zoneID, lockID)
		},
	)
	if err != nil {
		logging.Warn(ctx, "UnlockPool failed: %v", err)
	}

	return err
}

////////////////////////////////////////////////////////////////////////////////

func CreateStorage(
	config *pools_config.PoolsConfig,
	db *persistence.YDBClient,
) (Storage, error) {

	if config.GetMaxActiveSlots() == 0 {
		return nil, fmt.Errorf(
			"MaxActiveSlots should be defined, config=%v",
			config,
		)
	}

	if config.GetMaxBaseDisksInflight() == 0 {
		return nil, fmt.Errorf(
			"MaxBaseDisksInflight inflight should be defined, config=%v",
			config,
		)
	}

	if config.GetMaxBaseDiskUnits() == 0 {
		return nil, fmt.Errorf(
			"MaxBaseDiskUnits should be defined, config=%v",
			config,
		)
	}

	// Default value.
	takeBaseDisksToScheduleParallelism := 1
	if config.GetTakeBaseDisksToScheduleParallelism() != 0 {
		takeBaseDisksToScheduleParallelism = int(config.GetTakeBaseDisksToScheduleParallelism())
	}

	return &storageYDB{
		db:                                 db,
		tablesPath:                         db.AbsolutePath(config.GetStorageFolder()),
		maxActiveSlots:                     uint64(config.GetMaxActiveSlots()),
		maxBaseDisksInflight:               uint64(config.GetMaxBaseDisksInflight()),
		maxBaseDiskUnits:                   uint64(config.GetMaxBaseDiskUnits()),
		takeBaseDisksToScheduleParallelism: takeBaseDisksToScheduleParallelism,
	}, nil
}
