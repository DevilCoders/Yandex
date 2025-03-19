package images

import (
	"context"
	"time"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/clients/nbs"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/clients/snapshot"
	performance_config "a.yandex-team.ru/cloud/disk_manager/internal/pkg/performance/config"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/resources"
	images_config "a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/images/config"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/pools"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks"
)

////////////////////////////////////////////////////////////////////////////////

func Register(
	ctx context.Context,
	config *images_config.ImagesConfig,
	performanceConfig *performance_config.PerformanceConfig,
	taskRegistry *tasks.Registry,
	taskScheduler tasks.Scheduler,
	storage resources.Storage,
	nbsFactory nbs.Factory,
	snapshotFactory snapshot.Factory,
	poolService pools.Service,
) error {

	pingPeriod, err := time.ParseDuration(config.GetSnapshotTaskPingPeriod())
	if err != nil {
		return err
	}

	heartbeatTimeout, err := time.ParseDuration(config.GetSnapshotTaskHeartbeatTimeout())
	if err != nil {
		return err
	}

	backoffTimeout, err := time.ParseDuration(config.GetSnapshotTaskBackoffTimeout())
	if err != nil {
		return err
	}

	deletedImageExpirationTimeout, err := time.ParseDuration(config.GetDeletedImageExpirationTimeout())
	if err != nil {
		return err
	}

	clearDeletedImagesTaskScheduleInterval, err := time.ParseDuration(
		config.GetClearDeletedImagesTaskScheduleInterval(),
	)
	if err != nil {
		return err
	}

	err = taskRegistry.Register(ctx, "images.CreateImageFromURL", func() tasks.Task {
		return &createImageFromURLTask{
			scheduler:                taskScheduler,
			storage:                  storage,
			snapshotFactory:          snapshotFactory,
			snapshotPingPeriod:       pingPeriod,
			snapshotHeartbeatTimeout: heartbeatTimeout,
			snapshotBackoffTimeout:   backoffTimeout,
			poolService:              poolService,
		}
	})
	if err != nil {
		return err
	}

	err = taskRegistry.Register(ctx, "images.CreateImageFromImage", func() tasks.Task {
		return &createImageFromImageTask{
			performanceConfig:        performanceConfig,
			scheduler:                taskScheduler,
			storage:                  storage,
			snapshotFactory:          snapshotFactory,
			snapshotPingPeriod:       pingPeriod,
			snapshotHeartbeatTimeout: heartbeatTimeout,
			snapshotBackoffTimeout:   backoffTimeout,
			poolService:              poolService,
		}
	})
	if err != nil {
		return err
	}

	err = taskRegistry.Register(ctx, "images.CreateImageFromSnapshot", func() tasks.Task {
		return &createImageFromSnapshotTask{
			performanceConfig:        performanceConfig,
			scheduler:                taskScheduler,
			storage:                  storage,
			snapshotFactory:          snapshotFactory,
			snapshotPingPeriod:       pingPeriod,
			snapshotHeartbeatTimeout: heartbeatTimeout,
			snapshotBackoffTimeout:   backoffTimeout,
			poolService:              poolService,
		}
	})
	if err != nil {
		return err
	}

	err = taskRegistry.Register(ctx, "images.CreateImageFromDisk", func() tasks.Task {
		return &createImageFromDiskTask{
			performanceConfig:        performanceConfig,
			scheduler:                taskScheduler,
			storage:                  storage,
			nbsFactory:               nbsFactory,
			snapshotFactory:          snapshotFactory,
			snapshotPingPeriod:       pingPeriod,
			snapshotHeartbeatTimeout: heartbeatTimeout,
			snapshotBackoffTimeout:   backoffTimeout,
			poolService:              poolService,
		}
	})
	if err != nil {
		return err
	}

	err = taskRegistry.Register(ctx, "images.DeleteImage", func() tasks.Task {
		return &deleteImageTask{
			scheduler:                taskScheduler,
			storage:                  storage,
			snapshotFactory:          snapshotFactory,
			snapshotPingPeriod:       pingPeriod,
			snapshotHeartbeatTimeout: heartbeatTimeout,
			snapshotBackoffTimeout:   backoffTimeout,
			poolService:              poolService,
		}
	})
	if err != nil {
		return err
	}

	err = taskRegistry.Register(ctx, "images.ClearDeletedImages", func() tasks.Task {
		return &clearDeletedImagesTask{
			storage:           storage,
			expirationTimeout: deletedImageExpirationTimeout,
			limit:             int(config.GetClearDeletedImagesLimit()),
		}
	})
	if err != nil {
		return err
	}

	taskScheduler.ScheduleRegularTasks(
		ctx,
		"images.ClearDeletedImages",
		"",
		clearDeletedImagesTaskScheduleInterval,
		1,
	)

	return nil
}
