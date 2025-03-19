package tests

import (
	"context"
	"fmt"
	"math/rand"
	"os"
	"testing"
	"time"

	"github.com/stretchr/testify/require"

	nbs_client "a.yandex-team.ru/cloud/disk_manager/internal/pkg/clients/nbs"
	nbs_config "a.yandex-team.ru/cloud/disk_manager/internal/pkg/clients/nbs/config"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/dataplane/common"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/dataplane/nbs"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/dataplane/snapshot"
	snapshot_config "a.yandex-team.ru/cloud/disk_manager/internal/pkg/dataplane/snapshot/config"
	snapshot_storage "a.yandex-team.ru/cloud/disk_manager/internal/pkg/dataplane/snapshot/storage"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/dataplane/snapshot/storage/schema"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/dataplane/test"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/logging"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/monitoring/metrics"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/persistence"
	persistence_config "a.yandex-team.ru/cloud/disk_manager/internal/pkg/persistence/config"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/errors"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/types"
)

////////////////////////////////////////////////////////////////////////////////

const (
	blockSize           = uint32(4096)
	blocksInChunk       = uint32(16)
	chunkSize           = blocksInChunk * blockSize
	chunkCount          = uint32(222)
	blockCount          = uint64(blocksInChunk * chunkCount)
	readerCount         = 33
	writerCount         = 44
	chunksInflightLimit = 100
)

////////////////////////////////////////////////////////////////////////////////

func createContext() context.Context {
	return logging.SetLogger(
		context.Background(),
		logging.CreateStderrLogger(logging.DebugLevel),
	)
}

func createFactory(t *testing.T, ctx context.Context) nbs_client.Factory {
	rootCertsFile := os.Getenv("DISK_MANAGER_RECIPE_ROOT_CERTS_FILE")

	factory, err := nbs_client.CreateFactory(
		ctx,
		&nbs_config.ClientConfig{
			Zones: map[string]*nbs_config.Zone{
				"zone": {
					Endpoints: []string{
						fmt.Sprintf(
							"localhost:%v",
							os.Getenv("DISK_MANAGER_RECIPE_NBS_PORT"),
						),
						fmt.Sprintf(
							"localhost:%v",
							os.Getenv("DISK_MANAGER_RECIPE_NBS_PORT"),
						),
					},
				},
			},
			RootCertsFile: &rootCertsFile,
		},
		metrics.CreateEmptyRegistry(),
	)
	require.NoError(t, err)

	return factory
}

func createDisk(
	t *testing.T,
	ctx context.Context,
	factory nbs_client.Factory,
	disk *types.Disk,
) {

	client, err := factory.GetClient(ctx, disk.ZoneId)
	require.NoError(t, err)

	err = client.Create(ctx, nbs_client.CreateDiskParams{
		ID:          disk.DiskId,
		BlocksCount: blockCount,
		BlockSize:   blockSize,
		Kind:        types.DiskKind_DISK_KIND_SSD,
	})
	require.NoError(t, err)
}

func createDiskSource(
	t *testing.T,
	ctx context.Context,
	factory nbs_client.Factory,
	disk *types.Disk,
) common.Source {

	client, err := factory.GetClient(ctx, disk.ZoneId)
	require.NoError(t, err)

	err = client.CreateCheckpoint(ctx, disk.DiskId, "checkpoint")
	require.NoError(t, err)

	source, err := nbs.CreateDiskSource(
		ctx,
		factory,
		disk,
		"",
		"checkpoint",
		chunkSize,
		true, // useGetChangedBlocks
	)
	require.NoError(t, err)

	return source
}

////////////////////////////////////////////////////////////////////////////////

func createYDB(ctx context.Context) (*persistence.YDBClient, error) {
	endpoint := fmt.Sprintf(
		"localhost:%v",
		os.Getenv("DISK_MANAGER_RECIPE_KIKIMR_PORT"),
	)
	database := "/Root"
	connectionTimeout := "10s"

	return persistence.CreateYDBClient(
		ctx,
		&persistence_config.PersistenceConfig{
			Endpoint:          &endpoint,
			Database:          &database,
			ConnectionTimeout: &connectionTimeout,
		},
	)
}

func createStorage(
	t *testing.T,
	ctx context.Context,
	db *persistence.YDBClient,
) snapshot_storage.Storage {

	storageFolder := fmt.Sprintf("transfer_integration_test/%v", t.Name())
	deleteWorkerCount := uint32(10)
	shallowCopyWorkerCount := uint32(10)
	shallowCopyInflightLimit := uint32(100)
	shardCount := uint64(2)
	s3Bucket := "test"
	chunkBlobsS3KeyPrefix := t.Name()

	config := &snapshot_config.SnapshotConfig{
		StorageFolder:             &storageFolder,
		DeleteWorkerCount:         &deleteWorkerCount,
		ShallowCopyWorkerCount:    &shallowCopyWorkerCount,
		ShallowCopyInflightLimit:  &shallowCopyInflightLimit,
		ChunkBlobsTableShardCount: &shardCount,
		ChunkMapTableShardCount:   &shardCount,
		S3Bucket:                  &s3Bucket,
		ChunkBlobsS3KeyPrefix:     &chunkBlobsS3KeyPrefix,
	}

	s3, err := test.NewS3Client()
	require.NoError(t, err)

	err = schema.Create(ctx, config, db, s3)
	require.NoError(t, err)

	storage, err := snapshot_storage.CreateStorage(
		config,
		metrics.CreateEmptyRegistry(),
		db,
		s3,
	)
	require.NoError(t, err)

	return storage
}

////////////////////////////////////////////////////////////////////////////////

type Resource struct {
	createSource func() common.Source
	createTarget func() common.Target
}

func (r *Resource) fill(
	t *testing.T,
	ctx context.Context,
	chunks []common.Chunk,
) error {

	logging.Info(ctx, "Filling resource...")

	target := r.createTarget()
	defer target.Close(ctx)

	for _, chunk := range chunks {
		err := target.Write(ctx, chunk)
		if err != nil {
			return err
		}
	}

	return nil
}

func (r *Resource) checkChunks(
	t *testing.T,
	ctx context.Context,
	expectedChunks []common.Chunk,
) {

	logging.Info(ctx, "Checking result...")

	source := r.createSource()
	defer source.Close(ctx)

	// TODO: hack that warms up source's caches.
	processedChunkIndices := make(chan uint32, 1)
	chunkIndices, _ := source.Next(ctx, 0, processedChunkIndices)
	for chunkIndex := range chunkIndices {
		processedChunkIndices <- chunkIndex
	}

	for _, expected := range expectedChunks {
		actual := common.Chunk{
			Index: expected.Index,
			Data:  make([]byte, chunkSize),
		}
		err := source.Read(ctx, &actual)
		require.NoError(t, err)

		require.Equal(t, expected.Index, actual.Index)
		require.Equal(t, expected.Zero, actual.Zero)
		if !expected.Zero {
			require.Equal(t, expected.Data, actual.Data)
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

type transferTestCase struct {
	name               string
	useS3              bool
	withRandomFailures bool
}

func testCases() []transferTestCase {
	return []transferTestCase{
		{
			name:               "store chunks in ydb without random failures",
			useS3:              false,
			withRandomFailures: false,
		},
		{
			name:               "store chunks in s3 without random failures",
			useS3:              true,
			withRandomFailures: false,
		},
		{
			name:               "store chunks in ydb with random failures",
			useS3:              false,
			withRandomFailures: true,
		},
		{
			name:               "store chunks in s3 with random failures",
			useS3:              true,
			withRandomFailures: true,
		},
	}
}

////////////////////////////////////////////////////////////////////////////////

func testTransfer(
	t *testing.T,
	ctx context.Context,
	from Resource,
	to Resource,
	withRandomFailures bool,
) {

	logging.Info(
		ctx,
		"Starting transfer test=%v, withRandomFailures=%v",
		t.Name(),
		withRandomFailures,
	)

	rand.Seed(time.Now().UnixNano())

	chunks := make([]common.Chunk, 0)
	for i := uint32(0); i < chunkCount; i++ {
		dice := rand.Intn(3)

		switch dice {
		case 0:
			data := make([]byte, chunkSize)
			rand.Read(data)
			chunk := common.Chunk{Index: i, Data: data}
			chunks = append(chunks, chunk)
		case 1:
			// Zero chunk.
			chunk := common.Chunk{Index: i, Zero: true}
			chunks = append(chunks, chunk)
		}
	}

	milestoneChunkIndex := uint32(0)

	attemptIndex := 0

	attempt := func() error {
		source := from.createSource()
		defer source.Close(ctx)

		target := to.createTarget()
		defer target.Close(ctx)

		logging.Info(
			ctx,
			"Attempt #%v, withRandomFailures=%v",
			attemptIndex,
			withRandomFailures,
		)

		transferCtx, cancelTransferCtx := context.WithCancel(ctx)
		defer cancelTransferCtx()

		if withRandomFailures {
			go func() {
				for {
					select {
					case <-transferCtx.Done():
						return
					case <-time.After(2 * time.Second):
						if rand.Intn(2) == 0 {
							logging.Info(ctx, "Cancelling transfer context...")
							cancelTransferCtx()
						}
					}
				}
			}()
		}

		err := common.Transfer(
			transferCtx,
			source,
			target,
			milestoneChunkIndex,
			func(ctx context.Context) error {
				milestoneChunkIndex = source.GetMilestoneChunkIndex()

				logging.Info(
					ctx,
					"Updating progress, milestoneChunkIndex=%v",
					milestoneChunkIndex,
				)

				if withRandomFailures && rand.Intn(4) == 0 {
					return &errors.RetriableError{
						Err: fmt.Errorf("emulated saveProgress error"),
					}
				}

				return nil
			},
			readerCount,
			writerCount,
			chunksInflightLimit,
			int(chunkSize),
		)
		if err != nil {
			if transferCtx.Err() != nil {
				return &errors.RetriableError{
					Err: fmt.Errorf("transfer context was cancelled"),
				}
			}

			return err
		}

		return nil
	}

	err := from.fill(t, ctx, chunks)
	require.NoError(t, err)

	for {
		err := attempt()
		if err == nil {
			to.checkChunks(t, ctx, chunks)
			return
		}

		if errors.CanRetry(err) {
			logging.Warn(ctx, "Attempt #%v failed: %v", attemptIndex, err)
			attemptIndex++
			continue
		}

		require.NoError(t, err)
	}
}

////////////////////////////////////////////////////////////////////////////////

func TestTransferFromDiskToDisk(t *testing.T) {
	for _, testCase := range testCases() {
		t.Run(testCase.name, func(t *testing.T) {
			ctx, cancelCtx := context.WithCancel(createContext())
			defer cancelCtx()

			factory := createFactory(t, ctx)

			diskID1 := fmt.Sprintf("%v_%v", t.Name(), 1)
			disk1 := &types.Disk{ZoneId: "zone", DiskId: diskID1}
			createDisk(t, ctx, factory, disk1)

			diskID2 := fmt.Sprintf("%v_%v", t.Name(), 2)
			disk2 := &types.Disk{ZoneId: "zone", DiskId: diskID2}
			createDisk(t, ctx, factory, disk2)

			from := Resource{
				createSource: func() common.Source {
					return createDiskSource(t, ctx, factory, disk1)
				},
				createTarget: func() common.Target {
					target, err := nbs.CreateDiskTarget(ctx, factory, disk1, chunkSize, false)
					require.NoError(t, err)
					return target
				},
			}

			to := Resource{
				createSource: func() common.Source {
					return createDiskSource(t, ctx, factory, disk2)
				},
				createTarget: func() common.Target {
					target, err := nbs.CreateDiskTarget(ctx, factory, disk2, chunkSize, false)
					require.NoError(t, err)
					return target
				},
			}

			testTransfer(t, ctx, from, to, testCase.withRandomFailures)
		})
	}
}

func TestTransferFromDiskToSnapshot(t *testing.T) {
	for _, testCase := range testCases() {
		t.Run(testCase.name, func(t *testing.T) {
			ctx, cancelCtx := context.WithCancel(createContext())
			defer cancelCtx()

			db, err := createYDB(ctx)
			require.NoError(t, err)
			defer db.Close(ctx)

			storage := createStorage(t, ctx, db)

			factory := createFactory(t, ctx)

			diskID := t.Name()
			disk := &types.Disk{ZoneId: "zone", DiskId: diskID}
			createDisk(t, ctx, factory, disk)

			snapshotID := t.Name()

			from := Resource{
				createSource: func() common.Source {
					return createDiskSource(t, ctx, factory, disk)
				},
				createTarget: func() common.Target {
					target, err := nbs.CreateDiskTarget(ctx, factory, disk, chunkSize, false)
					require.NoError(t, err)
					return target
				},
			}

			to := Resource{
				createSource: func() common.Source {
					return snapshot.CreateSnapshotSource(snapshotID, storage)
				},
				createTarget: func() common.Target {
					return snapshot.CreateSnapshotTarget(
						"", // uniqueID
						snapshotID,
						storage,
						false,
						false,
						testCase.useS3,
					)
				},
			}

			testTransfer(t, ctx, from, to, testCase.withRandomFailures)
		})
	}
}

func TestTransferFromSnapshotToDisk(t *testing.T) {
	for _, testCase := range testCases() {
		t.Run(testCase.name, func(t *testing.T) {
			ctx, cancelCtx := context.WithCancel(createContext())
			defer cancelCtx()

			db, err := createYDB(ctx)
			require.NoError(t, err)
			defer db.Close(ctx)

			storage := createStorage(t, ctx, db)

			factory := createFactory(t, ctx)

			diskID := t.Name()
			disk := &types.Disk{ZoneId: "zone", DiskId: diskID}
			createDisk(t, ctx, factory, disk)

			snapshotID := t.Name()

			from := Resource{
				createSource: func() common.Source {
					return snapshot.CreateSnapshotSource(snapshotID, storage)
				},
				createTarget: func() common.Target {
					return snapshot.CreateSnapshotTarget(
						"", // uniqueID
						snapshotID,
						storage,
						false,
						false,
						testCase.useS3,
					)
				},
			}

			to := Resource{
				createSource: func() common.Source {
					return createDiskSource(t, ctx, factory, disk)
				},
				createTarget: func() common.Target {
					target, err := nbs.CreateDiskTarget(ctx, factory, disk, chunkSize, false)
					require.NoError(t, err)
					return target
				},
			}

			testTransfer(t, ctx, from, to, testCase.withRandomFailures)
		})
	}
}

func TestTransferFromSnapshotToSnapshot(t *testing.T) {
	for _, testCase := range testCases() {
		t.Run(testCase.name, func(t *testing.T) {
			ctx, cancelCtx := context.WithCancel(createContext())
			defer cancelCtx()

			db, err := createYDB(ctx)
			require.NoError(t, err)
			defer db.Close(ctx)

			storage := createStorage(t, ctx, db)

			snapshotID1 := fmt.Sprintf(t.Name(), 1)
			snapshotID2 := fmt.Sprintf(t.Name(), 2)

			from := Resource{
				createSource: func() common.Source {
					return snapshot.CreateSnapshotSource(snapshotID1, storage)
				},
				createTarget: func() common.Target {
					return snapshot.CreateSnapshotTarget(
						"", // uniqueID
						snapshotID1,
						storage,
						false,
						false,
						testCase.useS3,
					)
				},
			}

			to := Resource{
				createSource: func() common.Source {
					return snapshot.CreateSnapshotSource(snapshotID2, storage)
				},
				createTarget: func() common.Target {
					return snapshot.CreateSnapshotTarget(
						"", // uniqueID
						snapshotID2,
						storage,
						false,
						false,
						testCase.useS3,
					)
				},
			}

			testTransfer(t, ctx, from, to, testCase.withRandomFailures)
		})
	}
}
