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
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/clients/nbs/config"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/dataplane/common"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/dataplane/nbs"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/logging"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/monitoring/metrics"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/types"
)

////////////////////////////////////////////////////////////////////////////////

const (
	blockSize     = uint32(4096)
	blocksInChunk = uint32(8)
	chunkSize     = blocksInChunk * blockSize
	chunkCount    = uint32(33)
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
		&config.ClientConfig{
			Zones: map[string]*config.Zone{
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

////////////////////////////////////////////////////////////////////////////////

func checkChunks(
	t *testing.T,
	ctx context.Context,
	source common.Source,
	expectedChunks []common.Chunk,
) {

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

func TestNext(t *testing.T) {
	ctx := createContext()

	factory := createFactory(t, ctx)
	client, err := factory.GetClient(ctx, "zone")
	require.NoError(t, err)

	diskID := t.Name()
	disk := &types.Disk{ZoneId: "zone", DiskId: diskID}

	blockCount := uint64(blocksInChunk * chunkCount)
	err = client.Create(ctx, nbs_client.CreateDiskParams{
		ID:          diskID,
		BlocksCount: blockCount,
		BlockSize:   blockSize,
		Kind:        types.DiskKind_DISK_KIND_SSD,
	})
	require.NoError(t, err)

	session, err := client.MountRW(ctx, diskID)
	require.NoError(t, err)
	defer session.Close(ctx)

	var totalChunkIndices []uint32

	rand.Seed(time.Now().UnixNano())
	for blockIndex := uint64(0); blockIndex < blockCount; blockIndex++ {
		changed := false
		dice := rand.Intn(3)

		var err error
		switch dice {
		case 0:
			changed = true
			bytes := make([]byte, blockSize)
			err = session.Write(ctx, blockIndex, bytes)
		case 1:
			changed = true
			err = session.Zero(ctx, blockIndex, 1)
		}
		require.NoError(t, err)

		if changed {
			chunkIndex := uint32(blockIndex) / blocksInChunk

			size := len(totalChunkIndices)
			if size == 0 || totalChunkIndices[size-1] != chunkIndex {
				totalChunkIndices = append(totalChunkIndices, chunkIndex)
			}
		}
	}

	err = client.CreateCheckpoint(ctx, diskID, "checkpoint")
	require.NoError(t, err)

	milestoneChunkIndex := uint32(0)
	for milestoneChunkIndex < uint32(len(totalChunkIndices)) {
		logging.Info(
			ctx,
			"doing iteration with milestoneChunkIndex=%v",
			milestoneChunkIndex,
		)

		processedChunkIndices := make(chan uint32, 1)
		var actualChunkIndices []uint32

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
		defer source.Close(ctx)

		chunkIndices, errors := source.Next(
			ctx,
			milestoneChunkIndex,
			processedChunkIndices,
		)

		for chunkIndex := range chunkIndices {
			processedChunkIndices <- chunkIndex
			actualChunkIndices = append(actualChunkIndices, chunkIndex)
		}

		position := 0
		for i, chunkIndex := range totalChunkIndices {
			if chunkIndex >= milestoneChunkIndex {
				position = i
				break
			}
		}
		expectedChunkIndices := totalChunkIndices[position:]

		for err := range errors {
			require.NoError(t, err)
		}
		require.Equal(t, expectedChunkIndices, actualChunkIndices)

		milestoneChunkIndex++
	}
}

func TestReadWrite(t *testing.T) {
	ctx := createContext()

	factory := createFactory(t, ctx)
	client, err := factory.GetClient(ctx, "zone")
	require.NoError(t, err)

	diskID := t.Name()
	disk := &types.Disk{ZoneId: "zone", DiskId: diskID}

	err = client.Create(ctx, nbs_client.CreateDiskParams{
		ID:          diskID,
		BlocksCount: uint64(blocksInChunk * chunkCount),
		BlockSize:   blockSize,
		Kind:        types.DiskKind_DISK_KIND_SSD,
	})
	require.NoError(t, err)

	target, err := nbs.CreateDiskTarget(ctx, factory, disk, chunkSize, false)
	require.NoError(t, err)
	defer target.Close(ctx)

	chunks := make([]common.Chunk, 0)
	for i := uint32(0); i < chunkCount; i++ {
		var chunk common.Chunk

		if rand.Intn(2) == 1 {
			data := make([]byte, chunkSize)
			rand.Read(data)
			chunk = common.Chunk{Index: i, Data: data}
		} else {
			// Zero chunk.
			chunk = common.Chunk{Index: i, Zero: true}
		}

		err = target.Write(ctx, chunk)
		require.NoError(t, err)

		chunks = append(chunks, chunk)
	}

	err = client.CreateCheckpoint(ctx, diskID, "checkpoint")
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
	defer source.Close(ctx)

	checkChunks(t, ctx, source, chunks)

	sourceChunkCount, err := source.GetChunkCount(ctx)
	require.NoError(t, err)
	require.Equal(t, chunkCount, sourceChunkCount)
}
