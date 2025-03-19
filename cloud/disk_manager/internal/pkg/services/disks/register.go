package disks

import (
	"context"
	"time"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/clients/nbs"
	performance_config "a.yandex-team.ru/cloud/disk_manager/internal/pkg/performance/config"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/resources"
	disks_config "a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/disks/config"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/pools"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/transfer"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks"
)

////////////////////////////////////////////////////////////////////////////////

func Register(
	ctx context.Context,
	config *disks_config.DisksConfig,
	performanceConfig *performance_config.PerformanceConfig,
	storage resources.Storage,
	taskRegistry *tasks.Registry,
	taskScheduler tasks.Scheduler,
	poolService pools.Service,
	nbsFactory nbs.Factory,
	transferService transfer.Service,
) error {

	deletedDiskExpirationTimeout, err := time.ParseDuration(
		config.GetDeletedDiskExpirationTimeout(),
	)
	if err != nil {
		return err
	}

	clearDeletedDisksTaskScheduleInterval, err := time.ParseDuration(
		config.GetClearDeletedDisksTaskScheduleInterval(),
	)
	if err != nil {
		return err
	}

	err = taskRegistry.Register(ctx, "disks.CreateEmptyDisk", func() tasks.Task {
		return &createEmptyDiskTask{
			storage:    storage,
			scheduler:  taskScheduler,
			nbsFactory: nbsFactory,
		}
	})
	if err != nil {
		return err
	}

	err = taskRegistry.Register(ctx, "disks.CreateOverlayDisk", func() tasks.Task {
		return &createOverlayDiskTask{
			storage:     storage,
			scheduler:   taskScheduler,
			poolService: poolService,
			nbsFactory:  nbsFactory,
		}
	})
	if err != nil {
		return err
	}

	err = taskRegistry.Register(ctx, "disks.CreateDiskFromImage", func() tasks.Task {
		return &createDiskFromImageTask{
			performanceConfig: performanceConfig,
			storage:           storage,
			scheduler:         taskScheduler,
			transferService:   transferService,
			nbsFactory:        nbsFactory,
		}
	})
	if err != nil {
		return err
	}

	err = taskRegistry.Register(ctx, "disks.CreateDiskFromSnapshot", func() tasks.Task {
		return &createDiskFromSnapshotTask{
			performanceConfig: performanceConfig,
			storage:           storage,
			scheduler:         taskScheduler,
			transferService:   transferService,
			nbsFactory:        nbsFactory,
		}
	})
	if err != nil {
		return err
	}

	err = taskRegistry.Register(ctx, "disks.DeleteDisk", func() tasks.Task {
		return &deleteDiskTask{
			storage:     storage,
			scheduler:   taskScheduler,
			poolService: poolService,
			nbsFactory:  nbsFactory,
		}
	})
	if err != nil {
		return err
	}

	err = taskRegistry.Register(ctx, "disks.ResizeDisk", func() tasks.Task {
		return &resizeDiskTask{
			nbsFactory: nbsFactory,
		}
	})
	if err != nil {
		return err
	}

	err = taskRegistry.Register(ctx, "disks.AlterDisk", func() tasks.Task {
		return &alterDiskTask{
			nbsFactory: nbsFactory,
		}
	})
	if err != nil {
		return err
	}

	err = taskRegistry.Register(ctx, "disks.AssignDisk", func() tasks.Task {
		return &assignDiskTask{
			nbsFactory: nbsFactory,
		}
	})
	if err != nil {
		return err
	}

	err = taskRegistry.Register(ctx, "disks.UnassignDisk", func() tasks.Task {
		return &unassignDiskTask{
			nbsFactory: nbsFactory,
		}
	})
	if err != nil {
		return err
	}

	err = taskRegistry.Register(ctx, "disks.ClearDeletedDisks", func() tasks.Task {
		return &clearDeletedDisksTask{
			storage:           storage,
			expirationTimeout: deletedDiskExpirationTimeout,
			limit:             int(config.GetClearDeletedDisksLimit()),
		}
	})
	if err != nil {
		return err
	}

	err = taskRegistry.Register(ctx, "disks.MigrateDisk", func() tasks.Task {
		return &migrateDiskTask{}
	})
	if err != nil {
		return err
	}

	taskScheduler.ScheduleRegularTasks(
		ctx,
		"disks.ClearDeletedDisks",
		"",
		clearDeletedDisksTaskScheduleInterval,
		1,
	)

	return nil
}
