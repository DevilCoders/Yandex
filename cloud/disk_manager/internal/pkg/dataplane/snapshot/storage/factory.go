package storage

import (
	snapshot_config "a.yandex-team.ru/cloud/disk_manager/internal/pkg/dataplane/snapshot/config"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/dataplane/snapshot/storage/chunks"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/dataplane/snapshot/storage/metrics"
	common_metrics "a.yandex-team.ru/cloud/disk_manager/internal/pkg/monitoring/metrics"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/persistence"
)

////////////////////////////////////////////////////////////////////////////////

func CreateStorage(
	config *snapshot_config.SnapshotConfig,
	metricsRegistry common_metrics.Registry,
	db *persistence.YDBClient,
	s3 *persistence.S3Client,
) (Storage, error) {

	tablesPath := db.AbsolutePath(config.GetStorageFolder())
	metrics := metrics.New(metricsRegistry)

	var chunkStorageS3 *chunks.StorageS3
	// TODO: remove when s3 will always be initialized.
	if s3 != nil {
		chunkStorageS3 = chunks.NewStorageS3(
			db,
			s3,
			config.GetS3Bucket(),
			config.GetChunkBlobsS3KeyPrefix(),
			tablesPath,
			metrics,
		)
	}

	return &storageYDB{
		db:                       db,
		tablesPath:               tablesPath,
		metrics:                  metrics,
		deleteWorkerCount:        int(config.GetDeleteWorkerCount()),
		shallowCopyWorkerCount:   int(config.GetShallowCopyWorkerCount()),
		shallowCopyInflightLimit: int(config.GetShallowCopyInflightLimit()),
		chunkCompression:         config.GetChunkCompression(),
		chunkStorageYDB:          chunks.NewStorageYDB(db, tablesPath, metrics),
		chunkStorageS3:           chunkStorageS3,
	}, nil
}

////////////////////////////////////////////////////////////////////////////////

func CreateLegacyStorage(
	config *snapshot_config.SnapshotConfig,
	metricsRegistry common_metrics.Registry,
	db *persistence.YDBClient,
) (Storage, error) {

	return &legacyStorage{
		db:         db,
		tablesPath: db.AbsolutePath(config.GetLegacyStorageFolder()),
		metrics:    metrics.New(metricsRegistry),
	}, nil
}
