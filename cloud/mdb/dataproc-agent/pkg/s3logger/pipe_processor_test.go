package s3logger

import (
	"context"
	"errors"
	"strings"
	"testing"
	"time"

	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/dataproc-agent/pkg/s3logger/mocks"
	"a.yandex-team.ru/library/go/core/log/nop"
)

func setupTest(t *testing.T, config Config) (*PipeProcessor, *mocks.MockChunkUploader) {
	if config.MaxPoolSize == 0 {
		config.MaxPoolSize = 1024
	}
	ctrl := gomock.NewController(t)
	uploader := mocks.NewMockChunkUploader(ctrl)
	processor := newPipeProcessor(uploader, config, &nop.Logger{})
	processor.startBGSync.Do(func() {}) // ensure background sync won't be started in tests
	return processor, uploader
}

var unit = 200 * time.Microsecond

func TestOutputIsCached(t *testing.T) {
	var err error
	var n int64

	processor, uploader := setupTest(t, Config{ChunkSize: 128})

	uploader.EXPECT().UploadChunk(gomock.Any(), 0, []byte("abcd")).Return(nil).Times(1)

	n, err = processor.ReadFrom(strings.NewReader("a"))
	require.Equal(t, int64(1), n)
	require.NoError(t, err)
	n, err = processor.ReadFrom(strings.NewReader("bc"))
	require.Equal(t, int64(2), n)
	require.NoError(t, err)
	n, err = processor.ReadFrom(strings.NewReader("d"))
	require.Equal(t, int64(1), n)
	require.NoError(t, err)
	err = processor.sync(context.Background())
	require.NoError(t, err)
}

func TestChunkUpdated(t *testing.T) {
	var err error
	var n int64

	processor, uploader := setupTest(t, Config{ChunkSize: 128})
	ctx := context.Background()

	uploader.EXPECT().UploadChunk(gomock.Any(), 0, []byte("a")).Return(nil).Times(1)
	n, err = processor.ReadFrom(strings.NewReader("a"))
	require.Equal(t, int64(1), n)
	require.NoError(t, err)
	err = processor.sync(ctx)
	require.NoError(t, err)

	uploader.EXPECT().UploadChunk(gomock.Any(), 0, []byte("abc")).Return(nil).Times(1)
	n, err = processor.ReadFrom(strings.NewReader("bc"))
	require.Equal(t, int64(2), n)
	require.NoError(t, err)
	err = processor.sync(ctx)
	require.NoError(t, err)

	uploader.EXPECT().UploadChunk(gomock.Any(), 0, []byte("abcd")).Return(nil).Times(1)
	n, err = processor.ReadFrom(strings.NewReader("d"))
	require.Equal(t, int64(1), n)
	require.NoError(t, err)
	err = processor.sync(ctx)
	require.NoError(t, err)
}

func TestMultipleChunks(t *testing.T) {
	var err error
	var n int64

	processor, uploader := setupTest(t, Config{ChunkSize: 2})
	ctx := context.Background()

	uploader.EXPECT().UploadChunk(gomock.Any(), 0, []byte("a")).Return(nil).Times(1)
	n, err = processor.ReadFrom(strings.NewReader("a"))
	require.Equal(t, int64(1), n)
	require.NoError(t, err)
	err = processor.sync(ctx)
	require.NoError(t, err)

	uploader.EXPECT().UploadChunk(gomock.Any(), 0, []byte("ab")).Return(nil).Times(1)
	n, err = processor.ReadFrom(strings.NewReader("b"))
	require.Equal(t, int64(1), n)
	require.NoError(t, err)
	err = processor.sync(ctx)
	require.NoError(t, err)

	uploader.EXPECT().UploadChunk(gomock.Any(), 1, []byte("c")).Return(nil).Times(1)
	n, err = processor.ReadFrom(strings.NewReader("c"))
	require.Equal(t, int64(1), n)
	require.NoError(t, err)
	err = processor.sync(ctx)
	require.NoError(t, err)
}

func TestSyncedChunksAreRemoved(t *testing.T) {
	var err error
	var n int64

	processor, uploader := setupTest(t, Config{ChunkSize: 2, SyncInterval: unit})
	ctx := context.Background()

	uploader.EXPECT().UploadChunk(gomock.Any(), gomock.Any(), gomock.Any()).Return(nil).Times(3)

	n, err = processor.ReadFrom(strings.NewReader("a"))
	require.Equal(t, int64(1), n)
	require.NoError(t, err)
	err = processor.sync(ctx)
	require.NoError(t, err)

	n, err = processor.ReadFrom(strings.NewReader("b"))
	require.Equal(t, int64(1), n)
	require.NoError(t, err)
	err = processor.sync(ctx)
	require.NoError(t, err)

	n, err = processor.ReadFrom(strings.NewReader("c"))
	require.Equal(t, int64(1), n)
	require.NoError(t, err)
	err = processor.sync(ctx)
	require.NoError(t, err)

	require.Equal(t, 1, processor.chunks.Len())
	require.Equal(t, "c", processor.chunks.Front().Value.(*LogChunk).asString())
}

func TestPoolSizeLimit(t *testing.T) {
	var err error
	var n int64

	processor, uploader := setupTest(t, Config{ChunkSize: 2, MaxPoolSize: 4})
	ctx := context.Background()
	uploader.EXPECT().UploadChunk(gomock.Any(), 0, []byte("ab")).Return(errors.New("some error")).Times(1)
	n, err = processor.ReadFrom(strings.NewReader("abc"))
	require.Equal(t, int64(3), n)
	require.NoError(t, err)
	err = processor.sync(ctx)
	require.Error(t, err)

	// when "de" is read chunk "ab" is removed from pool
	uploader.EXPECT().UploadChunk(gomock.Any(), 1, []byte("cd")).Return(nil).Times(1)
	uploader.EXPECT().UploadChunk(gomock.Any(), 2, []byte("e")).Return(nil).Times(1)
	n, err = processor.ReadFrom(strings.NewReader("de"))
	require.Equal(t, int64(2), n)
	require.NoError(t, err)
	err = processor.sync(ctx)
	require.NoError(t, err)
}
