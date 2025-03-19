package main

import (
	"context"
	"fmt"

	server_config "a.yandex-team.ru/cloud/disk_manager/internal/pkg/configs/server/config"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/persistence"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/resources"
	pools_storage "a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/pools/storage"
	tasks_storage "a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/storage"
)

////////////////////////////////////////////////////////////////////////////////

func runControlplane(
	ctx context.Context,
	config *server_config.ServerConfig,
	db *persistence.YDBClient,
) error {

	err := tasks_storage.CreateYDBTables(ctx, config.GetTasksConfig(), db)
	if err != nil {
		return fmt.Errorf("failed to create tables for tasks: %w", err)
	}

	err = pools_storage.CreateYDBTables(ctx, config.GetPoolsConfig(), db)
	if err != nil {
		return fmt.Errorf("failed to create tables for pools: %w", err)
	}

	fsStorageFolder := ""
	if config.GetFilesystemConfig() != nil {
		fsStorageFolder = config.GetFilesystemConfig().GetStorageFolder()
	}

	err = resources.CreateYDBTables(
		ctx,
		config.GetDisksConfig().GetStorageFolder(),
		config.GetImagesConfig().GetStorageFolder(),
		config.GetSnapshotsConfig().GetStorageFolder(),
		fsStorageFolder,
		config.GetPlacementGroupConfig().GetStorageFolder(),
		db,
	)
	if err != nil {
		return fmt.Errorf("failed to create tables for resources: %w", err)
	}

	return nil
}
