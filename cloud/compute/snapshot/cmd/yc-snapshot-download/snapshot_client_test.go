//nolint:errcheck
package main

import (
	"bytes"
	"context"
	"fmt"
	"io"
	"math/rand"
	"net"
	"net/http"
	"os"
	"strconv"
	"sync"
	"testing"

	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"
	"go.uber.org/zap"
	"go.uber.org/zap/zapcore"

	log "a.yandex-team.ru/cloud/compute/go-common/logging/ctxlog"
	"a.yandex-team.ru/cloud/compute/snapshot/internal/testhelper"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/client"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/common"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/config"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/server"
)

var NAME = "name"

func init() {
	testhelper.InitConfig()
}

type BufferCloser struct {
	*bytes.Buffer
}

func (b BufferCloser) Close() error {
	return nil
}

type testBuffer struct {
	data   []byte
	offset int
}

var (
	_ io.WriteCloser = &testBuffer{}
	_ io.WriterAt    = &testBuffer{}
)

func (b *testBuffer) Reset(size int) {
	b.data = make([]byte, size)
	b.offset = 0
}

func (b *testBuffer) Close() error {
	return nil
}

func (b testBuffer) Write(d []byte) (int, error) {
	copied := copy(b.data[b.offset:], d)
	if copied != len(d) {
		return copied, fmt.Errorf("write out of size")
	}
	return copied, nil
}

func (b *testBuffer) WriteAt(p []byte, off int64) (n int, err error) {
	copied := copy(b.data[off:], p)
	if copied != len(p) {
		return copied, fmt.Errorf("writeat of size")
	}
	return copied, nil
}

func TestWriteSnapshot(t *testing.T) {
	if testing.Short() {
		t.Skip("Skipped in short mode")
	}

	grpclistener, err := net.Listen("tcp", ":0")
	require.NoError(t, err)
	defer grpclistener.Close()

	httplistener, err := net.Listen("tcp", ":0")
	require.NoError(t, err)
	defer httplistener.Close()

	debuglistener, err := net.Listen("tcp", ":0")
	require.NoError(t, err)
	defer debuglistener.Close()

	go func() {
		_ = http.Serve(debuglistener, http.DefaultServeMux)
	}()

	// create encoder config
	// set configured logger as global
	// attach the logger to context
	encCfg := log.NewJournaldConfig().EncoderConfig
	encCfg.TimeKey = "T"
	encCfg.EncodeLevel = zapcore.CapitalLevelEncoder
	logger := zap.New(zapcore.NewCore(zapcore.NewConsoleEncoder(encCfg), os.Stdout, zapcore.DebugLevel))
	zap.ReplaceGlobals(logger)
	ctx := log.WithLogger(context.Background(), logger)
	ctx, cancel := context.WithCancel(ctx)

	conf, err := config.GetConfig()
	require.NoError(t, err, "unable to get configuration")

	conf.General.Tmpdir = os.TempDir()
	s, err := server.NewServer(ctx, &conf)
	require.NoError(t, err)
	require.NotNil(t, s, "Server is nil")
	defer s.Close(ctx)
	go func() {
		require.NoError(t, s.Start(server.Listeners{GRPCListener: grpclistener, HTTPListener: httplistener}))
	}()
	go func() {
		<-server.SignalListener
		s.Stop()
		cancel()
	}()
	defer s.Stop()

	c, err := client.NewClient(ctx, grpclistener.Addr().String(), false, nil)
	require.NoError(t, err, "Failed to create client")
	require.NotNil(t, c, "Client is nil")
	defer func() { assert.NoError(t, c.Close()) }()
	var id string

	t.Log("Create snapshot")

	BlockSize := 4 * 1024
	SnapshotSize := 100 * BlockSize
	assert.Equal(t, 0, SnapshotSize%BlockSize)

	data := make([]byte, SnapshotSize)

	emptyBlocks := []int{0, 50, 51, 90}
	rand.Read(data)
	zeroBlock := make([]byte, BlockSize)
	for _, emptyBlockIndex := range emptyBlocks {
		copy(data[BlockSize*emptyBlockIndex:], zeroBlock)
	}

	id, err = c.CreateSnapshot(ctx, &common.CreationInfo{
		CreationMetadata: common.CreationMetadata{
			UpdatableMetadata: common.UpdatableMetadata{
				Metadata:    "meta",
				Description: "desc",
			},
			ProjectID: "project",
			Name:      "test-snapshot-name-" + strconv.Itoa(rand.Int()),
		},
		Base: "",
		Size: int64(SnapshotSize),
		Disk: "sda",
	})
	require.NoError(t, err)
	require.NotEqual(t, "", id)
	defer c.DeleteSnapshot(ctx, &common.DeleteRequest{ID: id})

	var wg sync.WaitGroup

	for offset := 0; offset < SnapshotSize; offset += BlockSize {
		wg.Add(1)

		go func(dataBlock []byte, offset int64) {
			defer wg.Done()
			err = c.WriteSnapshot(ctx, id, dataBlock, offset)
			assert.NoError(t, err)
		}(data[offset:offset+BlockSize], int64(offset))
	}

	wg.Wait()

	err = c.CommitSnapshot(ctx, id)
	t.Log("Snapshot created succesfully")
	assert.NoError(t, err)

	t.Run("Test snapshot download", func(t *testing.T) {
		t.Run("Linear writer", func(t *testing.T) {
			t.Parallel()
			reader, err := c.OpenSnapshot(ctx, id)
			assert.NoError(t, err)

			linearBuffer := BufferCloser{}
			linearBuffer.Buffer = &bytes.Buffer{}
			err = writeSnapshot(ctx, linearBuffer, reader)
			assert.NoError(t, err)
			assert.Equal(t, data, linearBuffer.Bytes())
		})

		t.Run("WriteAt writer", func(t *testing.T) {
			t.Parallel()

			// Fill arr for detect non writed zero blocks
			localData := make([]byte, SnapshotSize)
			for index := range localData {
				localData[index] = 255
			}

			reader, err := c.OpenSnapshot(ctx, id)
			assert.NoError(t, err)
			writerBuffer := &testBuffer{}
			writerBuffer.data = localData
			err = writeSnapshot(ctx, writerBuffer, reader)
			assert.NoError(t, err)

			onesBlock := make([]byte, BlockSize)
			for index := range onesBlock {
				onesBlock[index] = 255
			}
			for _, emptyBlockIndex := range emptyBlocks {
				block := localData[BlockSize*emptyBlockIndex : BlockSize*(emptyBlockIndex+1)]
				assert.Equal(t, block, onesBlock)

				// fill zero - for compare data
				for index := range block {
					block[index] = 0
				}
			}

			assert.Equal(t, data, writerBuffer.data)
		})
	})

}
