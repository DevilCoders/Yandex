package main

import (
	"context"
	"fmt"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/auth"
	server_config "a.yandex-team.ru/cloud/disk_manager/internal/pkg/configs/server/config"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/dataplane/snapshot/storage/schema"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/persistence"
	tasks_storage "a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/storage"
)

/////////////////////////////////////////////////////////////////////////////////

func runDataplane(
	ctx context.Context,
	config *server_config.ServerConfig,
	creds auth.Credentials,
	db *persistence.YDBClient,
) error {

	err := tasks_storage.CreateYDBTables(ctx, config.GetTasksConfig(), db)
	if err != nil {
		return fmt.Errorf("failed to create tables for tasks: %w", err)
	}

	snapshotConfig := config.GetDataplaneConfig().GetSnapshotConfig()

	snapshotDB, err := persistence.CreateYDBClient(
		ctx,
		snapshotConfig.GetPersistenceConfig(),
		persistence.WithCredentials(creds),
	)
	if err != nil {
		return fmt.Errorf("failed to connect to snapshot DB: %w", err)
	}
	defer snapshotDB.Close(ctx)

	s3Config := snapshotConfig.GetPersistenceConfig().GetS3Config()
	var s3 *persistence.S3Client
	// TODO: remove when s3 will always be initialized.
	if s3Config != nil {
		s3, err = persistence.NewS3ClientFromConfig(s3Config)
		if err != nil {
			return fmt.Errorf("failed to connect to S3: %w", err)
		}
	}

	err = schema.Create(ctx, snapshotConfig, snapshotDB, s3)
	if err != nil {
		return fmt.Errorf("failed to create tables for snapshot storage: %w", err)
	}

	return nil
}
