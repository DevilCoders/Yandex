package chunks

import (
	"context"
	"fmt"
	"os"
	"testing"

	"github.com/stretchr/testify/require"
	ydb_table "github.com/ydb-platform/ydb-go-sdk/v3/table"
	ydb_types "github.com/ydb-platform/ydb-go-sdk/v3/table/types"

	snapshot_config "a.yandex-team.ru/cloud/disk_manager/internal/pkg/dataplane/snapshot/config"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/dataplane/snapshot/storage/metrics"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/dataplane/snapshot/storage/schema"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/dataplane/test"
	monitoring_metrics "a.yandex-team.ru/cloud/disk_manager/internal/pkg/monitoring/metrics"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/persistence"
	persistence_config "a.yandex-team.ru/cloud/disk_manager/internal/pkg/persistence/config"
)

////////////////////////////////////////////////////////////////////////////////

type TestCase struct {
	name  string
	useS3 bool
}

func testCases() []TestCase {
	return []TestCase{
		{
			name:  "ydb storage",
			useS3: false,
		},
		{
			name:  "s3 storage",
			useS3: true,
		},
	}
}

func newStorage(
	db *persistence.YDBClient,
	s3 *persistence.S3Client,
	config *snapshot_config.SnapshotConfig,
	useS3 bool,
) Storage {

	tablesPath := db.AbsolutePath(config.GetStorageFolder())
	metrics := metrics.New(monitoring_metrics.CreateEmptyRegistry())

	if useS3 {
		return NewStorageS3(
			db,
			s3,
			config.GetS3Bucket(),
			config.GetChunkBlobsS3KeyPrefix(),
			tablesPath,
			metrics,
		)
	} else {
		return NewStorageYDB(db, tablesPath, metrics)
	}
}

func setupEnvironment(
	t *testing.T,
) (context.Context, *persistence.YDBClient, *persistence.S3Client, *snapshot_config.SnapshotConfig) {

	ctx := test.NewContext()

	endpoint := fmt.Sprintf(
		"localhost:%v",
		os.Getenv("DISK_MANAGER_RECIPE_KIKIMR_PORT"),
	)
	database := "/Root"
	rootPath := "disk_manager"
	connectionTimeout := "10s"

	db, err := persistence.CreateYDBClient(
		ctx,
		&persistence_config.PersistenceConfig{
			Endpoint:          &endpoint,
			Database:          &database,
			RootPath:          &rootPath,
			ConnectionTimeout: &connectionTimeout,
		},
	)
	require.NoError(t, err)

	s3, err := test.NewS3Client()
	require.NoError(t, err)

	storageFolder := fmt.Sprintf("snapshot_chunk_storage_test/%v", t.Name())
	deleteWorkerCount := uint32(10)
	shallowCopyWorkerCount := uint32(10)
	shallowCopyInflightLimit := uint32(100)
	shardCount := uint64(2)
	compression := ""
	s3Bucket := "test"
	chunkBlobsS3KeyPrefix := t.Name()

	config := &snapshot_config.SnapshotConfig{
		StorageFolder:             &storageFolder,
		DeleteWorkerCount:         &deleteWorkerCount,
		ShallowCopyWorkerCount:    &shallowCopyWorkerCount,
		ShallowCopyInflightLimit:  &shallowCopyInflightLimit,
		ChunkBlobsTableShardCount: &shardCount,
		ChunkMapTableShardCount:   &shardCount,
		ChunkCompression:          &compression,
		S3Bucket:                  &s3Bucket,
		ChunkBlobsS3KeyPrefix:     &chunkBlobsS3KeyPrefix,
	}

	err = schema.Create(ctx, config, db, s3)
	require.NoError(t, err)

	return ctx, db, s3, config
}

////////////////////////////////////////////////////////////////////////////////

func chunkDataExists(
	t *testing.T,
	ctx context.Context,
	s3 *persistence.S3Client,
	db *persistence.YDBClient,
	config *snapshot_config.SnapshotConfig,
	chunkID string,
	useS3 bool,
) bool {

	if useS3 {
		return chunkDataExistsInS3(t, ctx, s3, config, chunkID)
	} else {
		return chunkDataExistsInYDB(t, ctx, db, config, chunkID)
	}
}

func chunkDataExistsInS3(
	t *testing.T,
	ctx context.Context,
	s3 *persistence.S3Client,
	config *snapshot_config.SnapshotConfig,
	chunkID string,
) bool {

	_, err := s3.GetObject(
		ctx,
		config.GetS3Bucket(),
		test.NewS3Key(config, chunkID),
	)
	if err == nil {
		return true
	}

	require.ErrorContains(t, err, "chunk not found")
	return false
}

func chunkDataExistsInYDB(
	t *testing.T,
	ctx context.Context,
	db *persistence.YDBClient,
	config *snapshot_config.SnapshotConfig,
	chunkID string,
) bool {

	res, err := db.ExecuteRO(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		declare $chunk_id as Utf8;

		select *
		from chunk_blobs
		where chunk_id = $chunk_id and referer = "";
	`, db.AbsolutePath(config.GetStorageFolder())), ydb_table.NewQueryParameters(
		ydb_table.ValueParam("$chunk_id", ydb_types.UTF8Value(chunkID)),
	))
	require.NoError(t, err)
	defer res.Close()

	if !res.NextResultSet(ctx) {
		return false
	}

	return res.NextRow()
}

func writeTestChunk(
	t *testing.T,
	ctx context.Context,
	storage Storage,
) (string, string, error) {

	referer := "testReferer"
	chunkID := "testChunkID"

	compressedDataLen, err := storage.WriteChunk(
		ctx,
		referer,
		chunkID,
		[]byte("test data"),
		"lz4", // compression
	)
	require.NoError(t, err)
	require.NotZero(t, compressedDataLen)

	return referer, chunkID, nil
}

func deleteMetadata(
	t *testing.T,
	ctx context.Context,
	db *persistence.YDBClient,
	config *snapshot_config.SnapshotConfig,
	chunkID string,
) {

	_, err := db.ExecuteRW(ctx, fmt.Sprintf(`
		--!syntax_v1
		pragma TablePathPrefix = "%v";
		pragma AnsiInForEmptyOrNullableItemsCollections;
		declare $chunk_id as Utf8;

		delete from chunk_blobs
		where chunk_id = $chunk_id and
			referer = "" and
			refcnt <= 1;
	`, db.AbsolutePath(config.GetStorageFolder())), ydb_table.NewQueryParameters(
		ydb_table.ValueParam("$chunk_id", ydb_types.UTF8Value(chunkID)),
	))
	require.NoError(t, err)
}

////////////////////////////////////////////////////////////////////////////////

func TestWriteIdempotency(t *testing.T) {
	for _, testCase := range testCases() {
		t.Run(testCase.name, func(t *testing.T) {
			ctx, db, s3, config := setupEnvironment(t)
			storage := newStorage(db, s3, config, testCase.useS3)

			for i := 0; i < 2; i++ {
				_, _, err := writeTestChunk(t, ctx, storage)
				require.NoError(t, err)
			}
		})
	}
}

func TestRefIdempotency(t *testing.T) {
	for _, testCase := range testCases() {
		t.Run(testCase.name, func(t *testing.T) {
			ctx, db, s3, config := setupEnvironment(t)
			storage := newStorage(db, s3, config, testCase.useS3)

			_, chunkID, err := writeTestChunk(t, ctx, storage)
			require.NoError(t, err)

			for i := 0; i < 2; i++ {
				err := storage.RefChunk(ctx, "newReferer", chunkID)
				require.NoError(t, err)
			}
		})
	}
}

func TestUnrefIdempotency(t *testing.T) {
	for _, testCase := range testCases() {
		t.Run(testCase.name, func(t *testing.T) {
			ctx, db, s3, config := setupEnvironment(t)
			storage := newStorage(db, s3, config, testCase.useS3)

			firstReferer, chunkID, err := writeTestChunk(t, ctx, storage)
			require.NoError(t, err)
			require.True(t, chunkDataExists(t, ctx, s3, db, config, chunkID, testCase.useS3))

			secondReferer := "secondReferer"
			err = storage.RefChunk(ctx, secondReferer, chunkID)
			require.NoError(t, err)
			require.True(t, chunkDataExists(t, ctx, s3, db, config, chunkID, testCase.useS3))

			for i := 0; i < 2; i++ {
				err = storage.UnrefChunk(ctx, firstReferer, chunkID)
				require.NoError(t, err)
				require.True(t, chunkDataExists(t, ctx, s3, db, config, chunkID, testCase.useS3))
			}

			for i := 0; i < 2; i++ {
				err = storage.UnrefChunk(ctx, secondReferer, chunkID)
				require.NoError(t, err)
				require.False(t, chunkDataExists(t, ctx, s3, db, config, chunkID, testCase.useS3))
			}
		})
	}
}

func TestLastUnrefDeletedMetadataButNotData(t *testing.T) {
	ctx, db, s3, config := setupEnvironment(t)
	storage := newStorage(db, s3, config, true)

	referer, chunkID, err := writeTestChunk(t, ctx, storage)
	require.NoError(t, err)
	require.True(t, chunkDataExists(t, ctx, s3, db, config, chunkID, true))

	deleteMetadata(t, ctx, db, config, chunkID)
	require.True(t, chunkDataExists(t, ctx, s3, db, config, chunkID, true))

	err = storage.UnrefChunk(ctx, referer, chunkID)
	require.NoError(t, err)
	require.False(t, chunkDataExists(t, ctx, s3, db, config, chunkID, true))
}
