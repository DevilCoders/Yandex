package main

import (
	"context"
	"fmt"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/auth"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/clients/nbs"
	server_config "a.yandex-team.ru/cloud/disk_manager/internal/pkg/configs/server/config"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/dataplane"
	snapshot_storage "a.yandex-team.ru/cloud/disk_manager/internal/pkg/dataplane/snapshot/storage"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/monitoring"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/persistence"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks"
	tasks_storage "a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/storage"
)

////////////////////////////////////////////////////////////////////////////////

func runDataplane(
	ctx context.Context,
	config *server_config.ServerConfig,
	hostname string,
	mon *monitoring.Monitoring,
	creds auth.Credentials,
	db *persistence.YDBClient,
	taskStorage tasks_storage.Storage,
	taskRegistry *tasks.Registry,
	taskScheduler tasks.Scheduler,
	nbsFactory nbs.Factory,
) error {

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

	snapshotMetricsRegistry := mon.CreateSolomonRegistry("snapshot_storage")

	s3Config := snapshotConfig.GetPersistenceConfig().GetS3Config()
	var s3 *persistence.S3Client
	// TODO: remove when s3 will always be initialized.
	if s3Config != nil {
		s3, err = persistence.NewS3ClientFromConfig(s3Config)
		if err != nil {
			return fmt.Errorf("failed to connect to S3: %w", err)
		}
	}

	snapshotStorage, err := snapshot_storage.CreateStorage(
		snapshotConfig,
		snapshotMetricsRegistry,
		snapshotDB,
		s3,
	)
	if err != nil {
		return fmt.Errorf("failed to create snapshot storage: %w", err)
	}

	snapshotLegacyStorage, err := snapshot_storage.CreateLegacyStorage(
		snapshotConfig,
		snapshotMetricsRegistry,
		snapshotDB,
	)
	if err != nil {
		return fmt.Errorf("failed to create snapshot legacy storage: %w", err)
	}

	err = dataplane.Register(
		ctx,
		taskRegistry,
		taskScheduler,
		nbsFactory,
		snapshotStorage,
		snapshotLegacyStorage,
		config.GetDataplaneConfig(),
	)
	if err != nil {
		return fmt.Errorf("failed to register dataplane tasks: %w", err)
	}

	runnerMetricsRegistry := mon.CreateSolomonRegistry("runners")

	err = tasks.StartRunners(
		ctx,
		taskStorage,
		taskRegistry,
		runnerMetricsRegistry,
		config.GetTasksConfig(),
		hostname,
	)
	if err != nil {
		return fmt.Errorf("failed to start runners: %w", err)
	}

	select {}
}
