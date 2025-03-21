package pools

import (
	"context"
	"time"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/clients/nbs"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/clients/snapshot"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/resources"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/pools/config"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/pools/storage"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/transfer"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks"
)

////////////////////////////////////////////////////////////////////////////////

func Register(
	ctx context.Context,
	config *config.PoolsConfig,
	taskRegistry *tasks.Registry,
	taskScheduler tasks.Scheduler,
	storage storage.Storage,
	nbsFactory nbs.Factory,
	snapshotFactory snapshot.Factory,
	transferService transfer.Service,
	resourceStorage resources.Storage,
) error {

	scheduleBaseDisksTaskScheduleInterval, err := time.ParseDuration(
		config.GetScheduleBaseDisksTaskScheduleInterval(),
	)
	if err != nil {
		return err
	}

	optimizeBaseDisksTaskScheduleInterval, err := time.ParseDuration(
		config.GetOptimizeBaseDisksTaskScheduleInterval(),
	)
	if err != nil {
		return err
	}

	minOptimizedPoolAge, err := time.ParseDuration(
		config.GetMinOptimizedPoolAge(),
	)
	if err != nil {
		return err
	}

	deleteBaseDisksTaskScheduleInterval, err := time.ParseDuration(
		config.GetDeleteBaseDisksTaskScheduleInterval(),
	)
	if err != nil {
		return err
	}

	deletedBaseDiskExpirationTimeout, err := time.ParseDuration(config.GetDeletedBaseDiskExpirationTimeout())
	if err != nil {
		return err
	}

	clearDeletedBaseDisksTaskScheduleInterval, err := time.ParseDuration(
		config.GetClearDeletedBaseDisksTaskScheduleInterval(),
	)
	if err != nil {
		return err
	}

	releasedSlotExpirationTimeout, err := time.ParseDuration(config.GetReleasedSlotExpirationTimeout())
	if err != nil {
		return err
	}

	clearReleasedSlotsTaskScheduleInterval, err := time.ParseDuration(
		config.GetClearReleasedSlotsTaskScheduleInterval(),
	)
	if err != nil {
		return err
	}

	err = taskRegistry.Register(ctx, "pools.AcquireBaseDisk", func() tasks.Task {
		return &acquireBaseDiskTask{
			scheduler: taskScheduler,
			storage:   storage,
		}
	})
	if err != nil {
		return err
	}

	err = taskRegistry.Register(ctx, "pools.CreateBaseDisk", func() tasks.Task {
		return &createBaseDiskTask{
			cloudID:  config.GetCloudId(),
			folderID: config.GetFolderId(),

			scheduler:       taskScheduler,
			storage:         storage,
			nbsFactory:      nbsFactory,
			transferService: transferService,
			resourceStorage: resourceStorage,
		}
	})
	if err != nil {
		return err
	}

	err = taskRegistry.Register(ctx, "pools.ReleaseBaseDisk", func() tasks.Task {
		return &releaseBaseDiskTask{
			storage: storage,
		}
	})
	if err != nil {
		return err
	}

	err = taskRegistry.Register(ctx, "pools.RebaseOverlayDisk", func() tasks.Task {
		return &rebaseOverlayDiskTask{
			storage:    storage,
			nbsFactory: nbsFactory,
		}
	})
	if err != nil {
		return err
	}

	err = taskRegistry.Register(ctx, "pools.ConfigurePool", func() tasks.Task {
		return &configurePoolTask{
			storage:         storage,
			snapshotFactory: snapshotFactory,
			resourceStorage: resourceStorage,
		}
	})
	if err != nil {
		return err
	}

	err = taskRegistry.Register(ctx, "pools.DeletePool", func() tasks.Task {
		return &deletePoolTask{
			storage: storage,
		}
	})
	if err != nil {
		return err
	}

	err = taskRegistry.Register(ctx, "pools.ImageDeleting", func() tasks.Task {
		return &imageDeletingTask{
			storage: storage,
		}
	})
	if err != nil {
		return err
	}

	err = taskRegistry.Register(ctx, "pools.RetireBaseDisk", func() tasks.Task {
		return &retireBaseDiskTask{
			scheduler:  taskScheduler,
			nbsFactory: nbsFactory,
			storage:    storage,
		}
	})
	if err != nil {
		return err
	}

	err = taskRegistry.Register(ctx, "pools.RetireBaseDisks", func() tasks.Task {
		return &retireBaseDisksTask{
			scheduler: taskScheduler,
			storage:   storage,
		}
	})
	if err != nil {
		return err
	}

	err = taskRegistry.Register(ctx, "pools.ScheduleBaseDisks", func() tasks.Task {
		return &scheduleBaseDisksTask{
			scheduler:                           taskScheduler,
			storage:                             storage,
			useDataplaneTasksForLegacySnapshots: config.GetUseDataplaneTasksForLegacySnapshots(),
		}
	})
	if err != nil {
		return err
	}

	err = taskRegistry.Register(ctx, "pools.OptimizeBaseDisks", func() tasks.Task {
		return &optimizeBaseDisksTask{
			scheduler:                               taskScheduler,
			storage:                                 storage,
			convertToImageSizedBaseDisksThreshold:   config.GetConvertToImageSizedBaseDiskThreshold(),
			convertToDefaultSizedBaseDisksThreshold: config.GetConvertToDefaultSizedBaseDiskThreshold(),
			minPoolAge:                              minOptimizedPoolAge,
		}
	})
	if err != nil {
		return err
	}

	err = taskRegistry.Register(ctx, "pools.DeleteBaseDisks", func() tasks.Task {
		return &deleteBaseDisksTask{
			storage:    storage,
			nbsFactory: nbsFactory,
			limit:      int(config.GetDeleteBaseDisksLimit()),
		}
	})
	if err != nil {
		return err
	}

	err = taskRegistry.Register(ctx, "pools.ClearDeletedBaseDisks", func() tasks.Task {
		return &clearDeletedBaseDisksTask{
			storage:           storage,
			expirationTimeout: deletedBaseDiskExpirationTimeout,
			limit:             int(config.GetClearDeletedBaseDisksLimit()),
		}
	})
	if err != nil {
		return err
	}

	err = taskRegistry.Register(ctx, "pools.ClearReleasedSlots", func() tasks.Task {
		return &clearReleasedSlotsTask{
			storage:           storage,
			expirationTimeout: releasedSlotExpirationTimeout,
			limit:             int(config.GetClearReleasedSlotsLimit()),
		}
	})
	if err != nil {
		return err
	}

	taskScheduler.ScheduleRegularTasks(
		ctx,
		"pools.ScheduleBaseDisks",
		"",
		scheduleBaseDisksTaskScheduleInterval,
		1,
	)

	if config.GetRegularBaseDiskOptimizationEnabled() {
		taskScheduler.ScheduleRegularTasks(
			ctx,
			"pools.OptimizeBaseDisks",
			"",
			optimizeBaseDisksTaskScheduleInterval,
			1,
		)
	}

	taskScheduler.ScheduleRegularTasks(
		ctx,
		"pools.DeleteBaseDisks",
		"",
		deleteBaseDisksTaskScheduleInterval,
		1,
	)

	taskScheduler.ScheduleRegularTasks(
		ctx,
		"pools.ClearDeletedBaseDisks",
		"",
		clearDeletedBaseDisksTaskScheduleInterval,
		1,
	)

	taskScheduler.ScheduleRegularTasks(
		ctx,
		"pools.ClearReleasedSlots",
		"",
		clearReleasedSlotsTaskScheduleInterval,
		1,
	)

	return nil
}
