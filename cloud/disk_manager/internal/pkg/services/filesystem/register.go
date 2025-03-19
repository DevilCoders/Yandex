package filesystem

import (
	"context"
	"time"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/clients/nfs"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/resources"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/filesystem/config"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks"
)

////////////////////////////////////////////////////////////////////////////////

func Register(
	ctx context.Context,
	config *config.FilesystemConfig,
	taskScheduler tasks.Scheduler,
	registry *tasks.Registry,
	storage resources.Storage,
	factory nfs.Factory,
) error {

	deletedFilesystemExpirationTimeout, err := time.ParseDuration(config.GetDeletedFilesystemExpirationTimeout())
	if err != nil {
		return err
	}

	clearDeletedFilesystemsTaskScheduleInterval, err := time.ParseDuration(
		config.GetClearDeletedFilesystemsTaskScheduleInterval(),
	)
	if err != nil {
		return err
	}

	err = registry.Register(ctx, "filesystem.CreateFilesystem", func() tasks.Task {
		return &createFilesystemTask{
			storage: storage,
			factory: factory,
		}
	})
	if err != nil {
		return err
	}

	err = registry.Register(ctx, "filesystem.DeleteFilesystem", func() tasks.Task {
		return &deleteFilesystemTask{
			storage: storage,
			factory: factory,
		}
	})
	if err != nil {
		return err
	}

	err = registry.Register(ctx, "filesystem.ResizeFilesystem", func() tasks.Task {
		return &resizeFilesystemTask{
			factory: factory,
		}
	})
	if err != nil {
		return err
	}

	err = registry.Register(ctx, "filesystems.ClearDeletedFilesystems", func() tasks.Task {
		return &clearDeletedFilesystemsTask{
			storage:           storage,
			expirationTimeout: deletedFilesystemExpirationTimeout,
			limit:             int(config.GetClearDeletedFilesystemsLimit()),
		}
	})
	if err != nil {
		return err
	}

	taskScheduler.ScheduleRegularTasks(
		ctx,
		"filesystems.ClearDeletedFilesystems",
		"",
		clearDeletedFilesystemsTaskScheduleInterval,
		1,
	)

	return nil
}
