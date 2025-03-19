package tests

import (
	"context"
	"fmt"
	"hash/crc32"
	"os"
	"strconv"
	"testing"
	"time"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/dataplane/common"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/logging"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/url"
)

////////////////////////////////////////////////////////////////////////////////

func getImageFileURL() string {
	port := os.Getenv("DISK_MANAGER_RECIPE_IMAGE_FILE_SERVER_PORT")
	return fmt.Sprintf("http://localhost:%v", port)
}

func getImageFileCrc32(t *testing.T) uint32 {
	value, err := strconv.ParseUint(os.Getenv("DISK_MANAGER_RECIPE_IMAGE_FILE_CRC32"), 10, 32)
	require.NoError(t, err)
	return uint32(value)
}

func getImageFileSize(t *testing.T) uint64 {
	value, err := strconv.ParseUint(os.Getenv("DISK_MANAGER_RECIPE_IMAGE_FILE_SIZE"), 10, 64)
	require.NoError(t, err)
	return value
}

////////////////////////////////////////////////////////////////////////////////

const (
	chunkSize = 4 * 1024 * 1024
)

////////////////////////////////////////////////////////////////////////////////

func createContext() context.Context {
	return logging.SetLogger(
		context.Background(),
		logging.CreateStderrLogger(logging.DebugLevel),
	)
}

////////////////////////////////////////////////////////////////////////////////

func checkChunks(
	t *testing.T,
	ctx context.Context,
	source common.Source,
) {

	chunkCount := uint32(getImageFileSize(t) / chunkSize)

	ctx, cancelCtx := context.WithTimeout(ctx, 300*time.Second)
	defer cancelCtx()

	chunks := make(map[uint32]common.Chunk)

	processedChunkIndices := make(chan uint32, chunkCount*2)

	chunkIndices, chunkIndicesErrors := source.Next(
		ctx,
		0,
		processedChunkIndices,
	)

	more := true
	var chunkIndex uint32

	for more {
		select {
		case chunkIndex, more = <-chunkIndices:
			if !more {
				break
			}

			chunk := common.Chunk{
				Index: chunkIndex,
				Data:  make([]byte, chunkSize),
			}

			err := source.Read(ctx, &chunk)
			require.NoError(t, err)

			chunks[chunkIndex] = chunk
		case err := <-chunkIndicesErrors:
			require.NoError(t, err)
		case <-ctx.Done():
			require.NoError(t, ctx.Err())
		}
	}

	require.Equal(t, chunkCount, uint32(len(chunks)))

	acc := crc32.NewIEEE()

	for i := uint32(0); i < chunkCount; i++ {
		data, ok := chunks[i]
		require.True(t, ok)

		_, err := acc.Write(data.Data)
		require.NoError(t, err)
	}

	require.Equal(t, getImageFileCrc32(t), acc.Sum32())
}

////////////////////////////////////////////////////////////////////////////////

func TestRawRead(t *testing.T) {
	ctx := createContext()

	source, err := url.CreateURLSource(
		ctx,
		getImageFileURL(),
		chunkSize,
	)
	require.NoError(t, err)
	defer source.Close(ctx)

	sourceChunkCount, err := source.GetChunkCount(ctx)
	chunkCount := uint32(getImageFileSize(t) / chunkSize)
	require.NoError(t, err)
	require.Equal(t, chunkCount, sourceChunkCount)

	checkChunks(t, ctx, source)
}
