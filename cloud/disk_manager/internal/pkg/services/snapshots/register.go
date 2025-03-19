package snapshots

import (
	"context"
	"time"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/clients/nbs"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/clients/snapshot"
	performance_config "a.yandex-team.ru/cloud/disk_manager/internal/pkg/performance/config"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/resources"
	snapshots_config "a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/snapshots/config"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks"
)

////////////////////////////////////////////////////////////////////////////////

func Register(
	ctx context.Context,
	config *snapshots_config.SnapshotsConfig,
	performanceConfig *performance_config.PerformanceConfig,
	taskRegistry *tasks.Registry,
	taskScheduler tasks.Scheduler,
	storage resources.Storage,
	nbsFactory nbs.Factory,
	snapshotFactory snapshot.Factory,
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

	deletedSnapshotExpirationTimeout, err := time.ParseDuration(config.GetDeletedSnapshotExpirationTimeout())
	if err != nil {
		return err
	}

	clearDeletedSnapshotsTaskScheduleInterval, err := time.ParseDuration(
		config.GetClearDeletedSnapshotsTaskScheduleInterval(),
	)
	if err != nil {
		return err
	}

	err = taskRegistry.Register(ctx, "snapshots.CreateSnapshotFromDisk", func() tasks.Task {
		return &createSnapshotFromDiskTask{
			performanceConfig:        performanceConfig,
			scheduler:                taskScheduler,
			storage:                  storage,
			nbsFactory:               nbsFactory,
			snapshotFactory:          snapshotFactory,
			snapshotPingPeriod:       pingPeriod,
			snapshotHeartbeatTimeout: heartbeatTimeout,
			snapshotBackoffTimeout:   backoffTimeout,
		}
	})
	if err != nil {
		return err
	}

	err = taskRegistry.Register(ctx, "snapshots.DeleteSnapshot", func() tasks.Task {
		return &deleteSnapshotTask{
			scheduler:                taskScheduler,
			storage:                  storage,
			nbsFactory:               nbsFactory,
			snapshotFactory:          snapshotFactory,
			snapshotPingPeriod:       pingPeriod,
			snapshotHeartbeatTimeout: heartbeatTimeout,
			snapshotBackoffTimeout:   backoffTimeout,
		}
	})
	if err != nil {
		return err
	}

	err = taskRegistry.Register(ctx, "snapshots.ClearDeletedSnapshots", func() tasks.Task {
		return &clearDeletedSnapshotsTask{
			storage:           storage,
			expirationTimeout: deletedSnapshotExpirationTimeout,
			limit:             int(config.GetClearDeletedSnapshotsLimit()),
		}
	})
	if err != nil {
		return err
	}

	taskScheduler.ScheduleRegularTasks(
		ctx,
		"snapshots.ClearDeletedSnapshots",
		"",
		clearDeletedSnapshotsTaskScheduleInterval,
		1,
	)

	return nil
}
