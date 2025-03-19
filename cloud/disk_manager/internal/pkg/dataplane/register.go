package dataplane

import (
	"context"
	"time"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/clients/nbs"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/dataplane/config"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/dataplane/snapshot/storage"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks"
)

////////////////////////////////////////////////////////////////////////////////

func Register(
	ctx context.Context,
	taskRegistry *tasks.Registry,
	taskScheduler tasks.Scheduler,
	nbsFactory nbs.Factory,
	storage storage.Storage,
	legacyStorage storage.Storage,
	config *config.DataplaneConfig,
) error {

	err := taskRegistry.Register(ctx, "dataplane.CreateSnapshotFromDisk", func() tasks.Task {
		return &createSnapshotFromDiskTask{
			nbsFactory: nbsFactory,
			storage:    storage,
			config:     config,
		}
	})
	if err != nil {
		return err
	}

	err = taskRegistry.Register(ctx, "dataplane.CreateSnapshotFromSnapshot", func() tasks.Task {
		return &createSnapshotFromSnapshotTask{
			storage: storage,
			config:  config,
		}
	})
	if err != nil {
		return err
	}

	err = taskRegistry.Register(ctx, "dataplane.CreateSnapshotFromURL", func() tasks.Task {
		return &createSnapshotFromURLTask{
			storage: storage,
			config:  config,
		}
	})
	if err != nil {
		return err
	}

	err = taskRegistry.Register(ctx, "dataplane.CreateSnapshotFromLegacySnapshot", func() tasks.Task {
		return &createSnapshotFromLegacySnapshotTask{
			storage:       storage,
			legacyStorage: legacyStorage,
			config:        config,
		}
	})
	if err != nil {
		return err
	}

	err = taskRegistry.Register(ctx, "dataplane.TransferFromSnapshotToDisk", func() tasks.Task {
		return &transferFromSnapshotToDiskTask{
			nbsFactory: nbsFactory,
			storage:    storage,
			config:     config,
		}
	})
	if err != nil {
		return err
	}

	err = taskRegistry.Register(ctx, "dataplane.TransferFromLegacySnapshotToDisk", func() tasks.Task {
		return &transferFromSnapshotToDiskTask{
			nbsFactory: nbsFactory,
			storage:    legacyStorage,
			config:     config,
		}
	})
	if err != nil {
		return err
	}

	err = taskRegistry.Register(ctx, "dataplane.TransferFromDiskToDisk", func() tasks.Task {
		return &transferFromDiskToDiskTask{
			nbsFactory: nbsFactory,
			config:     config,
		}
	})
	if err != nil {
		return err
	}

	err = taskRegistry.Register(ctx, "dataplane.DeleteSnapshot", func() tasks.Task {
		return &deleteSnapshotTask{
			storage: storage,
		}
	})
	if err != nil {
		return err
	}

	err = taskRegistry.Register(ctx, "dataplane.DeleteSnapshotData", func() tasks.Task {
		return &deleteSnapshotDataTask{
			storage: storage,
		}
	})
	if err != nil {
		return err
	}

	// TODO: reconsider this. Maybe we should extract regular tasks registration
	// code?
	if config != nil {
		snapshotCollectionTimeout, err := time.ParseDuration(
			config.GetSnapshotCollectionTimeout(),
		)
		if err != nil {
			return err
		}

		collectSnapshotsTaskScheduleInterval, err := time.ParseDuration(
			config.GetCollectSnapshotsTaskScheduleInterval(),
		)
		if err != nil {
			return err
		}

		err = taskRegistry.Register(
			ctx,
			"dataplane.CollectSnapshots",
			func() tasks.Task {
				return &collectSnapshotsTask{
					scheduler:                       taskScheduler,
					storage:                         storage,
					snapshotCollectionTimeout:       snapshotCollectionTimeout,
					snapshotCollectionInflightLimit: int(config.GetSnapshotCollectionInflightLimit()),
				}
			},
		)
		if err != nil {
			return err
		}

		taskScheduler.ScheduleRegularTasks(
			ctx,
			"dataplane.CollectSnapshots",
			"",
			collectSnapshotsTaskScheduleInterval,
			1,
		)

	}

	return nil
}
