//nolint:errcheck
package lib

import (
	"bytes"
	"context"
	"database/sql"
	"encoding/json"
	"errors"
	"fmt"
	"io"
	"io/ioutil"
	"log"
	"math/rand"
	"net"
	"net/http"
	"os"
	"path/filepath"
	"runtime"
	"strconv"
	"sync"
	"testing"
	"time"

	"github.com/gofrs/uuid"
	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"
	"go.uber.org/zap"
	"go.uber.org/zap/zaptest"
	"golang.org/x/xerrors"

	"a.yandex-team.ru/cloud/blockstore/public/sdk/go/client"
	kikimrprovider "a.yandex-team.ru/cloud/compute/go-common/database/kikimr"
	"a.yandex-team.ru/cloud/compute/go-common/logging/ctxlog"
	"a.yandex-team.ru/cloud/compute/go-common/tracing"
	"a.yandex-team.ru/cloud/compute/snapshot/internal/dockerprocess"
	"a.yandex-team.ru/cloud/compute/snapshot/internal/serializedlock"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/common"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/config"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/chunker"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/kikimr"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/misc"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/parallellimiter"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/proxy"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/server"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/storage"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/tasks"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/logging"
	"a.yandex-team.ru/library/go/test/yatest"
)

const (
	chunkSize    int64  = 4 * 1024 * 1024
	nbsBlockSize uint32 = 4 * 1024

	META      = "metadata is good!"
	DISK      = "home_video"
	ORG       = "MenInBlack Inc"
	PROJECT   = "projectX"
	DESC      = "description"
	productID = "product_id"
	imageID   = "image_id"

	darwin  = "darwin"
	windows = "windows"

	defaultDatabase = "default"
	defaultNBSConf  = "default"
)

var (
	pr            *proxy.Proxy
	NAME          = "name"
	convertURL    string
	redirectedURL string
	rawImageURL   string
)

type empty struct{}

type snapshot struct {
	common.SnapshotInfo
}

func init() {
	ctx, ctxCancel := createContext(nil, "init")
	defer ctxCancel()

	logger := ctxlog.G(ctx)

	defaultConfig := os.Getenv("TEST_CONFIG_PATH")
	if defaultConfig == "" {
		defaultConfig = "config.toml"
	}
	defaultConfig = yatest.SourcePath(defaultConfig)
	if err := config.LoadConfig(defaultConfig); err != nil {
		panic(err)
	}

	conf, err := config.GetConfig()
	if err != nil {
		panic(err)
	}

	_ = os.Remove(conf.QemuDockerProxy.SocketPath)
	l, err := net.Listen("unix", conf.QemuDockerProxy.SocketPath)
	if err != nil {
		logger.Fatal("Can't listen proxy: %v", zap.Error(err))
	}
	pr = proxy.NewProxy(conf.QemuDockerProxy.HostForDocker, conf.QemuDockerProxy.IDLengthBytes)
	go func() {
		err := pr.Serve(l)
		if err != http.ErrServerClosed {
			panic(err)
		}
	}()

	lServer, err := net.Listen("tcp", "127.0.0.1:0")
	if err != nil {
		panic(err)
	}

	convertURL = server.FileURL(lServer)
	redirectedURL = server.RedirectURL(lServer)
	rawImageURL = server.RawFileURL(lServer)

	go func() {
		err := server.NewServer().Serve(lServer)
		if err != http.ErrServerClosed {
			panic(err)
		}
	}()

	ydbPort, ok := os.LookupEnv("LOCAL_KIKIMR_KIKIMR_SERVER_PORT")
	if !ok {
		panic("Local testing Kikimr YDB port is not set")
	}
	defaultDB := conf.Kikimr[defaultDatabase]
	defaultDB.DBHost = fmt.Sprintf("localhost:%s", ydbPort)
	ydbRoot, ok := os.LookupEnv("LOCAL_KIKIMR_KIKIMR_ROOT")
	if !ok {
		panic("Local testing Kikimr YDB root is not set")
	}
	defaultDB.Root = "/" + ydbRoot
	defaultDB.DBName = "/" + ydbRoot
	conf.Kikimr[defaultDatabase] = defaultDB

	defaultNBS := conf.Nbs[defaultNBSConf]
	port, ok := os.LookupEnv("LOCAL_KIKIMR_INSECURE_NBS_SERVER_PORT")
	if !ok {
		panic("Local testing Kikimr NBS port is not set")
	}
	defaultNBS.Hosts = []string{fmt.Sprintf("%s:%s", "localhost", port)}
	conf.Nbs[defaultNBSConf] = defaultNBS

	st, err := PrepareStorage(ctx, &conf, misc.TableOpCreate|misc.TableOpDrop)
	if err != nil {
		panic(err)
	}
	defer st.Close()

	dockerprocess.SetDockerConfig(conf.Nbd.DockerConfig)

	rand.Seed(time.Now().UnixNano())
	NAME += strconv.Itoa(rand.Int())
}

type lockHolderMock struct{}

func (l lockHolderMock) Close(ctx context.Context) {
	// pass
}

type taskLimiterStub struct{}

func (taskLimiterStub) WaitQueue(ctx context.Context, _ parallellimiter.OperationDescriptor) (
	resCtx context.Context,
	resHolder storage.LockHolder,
	resErr error,
) {
	return ctx, lockHolderMock{}, nil
}

func TestMain(m *testing.M) {
	var result int
	conf, _ := config.GetConfig()
	if conf.Tracing.Address != "" {
		tracer, err := tracing.InitJaegerTracing(conf.Tracing.Config)
		if err != nil {
			panic(err)
		}
		defer func() {
			_ = tracer.Close()
			os.Exit(result)
		}()
	}
	m.Run()
}

func createContext(t *testing.T, name string) (context.Context, context.CancelFunc) {
	var logger *zap.Logger
	if t == nil {
		logger = logging.SetupCliLogging().WithOptions(zap.Development())
	} else {
		logger = zaptest.NewLogger(t, zaptest.WrapOptions(zap.Development()))
	}
	ctx := ctxlog.WithLogger(context.Background(), logger.Named(name))
	ctx, ctxCancel := context.WithCancel(ctx)
	return ctx, ctxCancel
}

func createTestContext(t *testing.T) (context.Context, context.CancelFunc) {
	return createContext(t, t.Name())
}

func makeUUID(t *testing.T) string {
	u, err := uuid.NewV4()
	require.NoError(t, err)
	return u.String()
}

func readBad(ctx context.Context, t *testing.T, ch *chunker.Chunker, sh *snapshot, data []byte) {
	assert := assert.New(t)
	_, err := ch.GetReader(ctx, sh.ID, 1)
	assert.EqualError(err, misc.ErrNoSuchChunk.Error())
	read := make([]byte, len(data)+1)

	rc, err := ch.GetReader(ctx, sh.ID, 0)
	assert.NoError(err)
	_, err = io.ReadFull(rc, read[:1])
	assert.NoError(err)
	assert.NoError(rc.Close())
}

func read(ctx context.Context, t *testing.T, ch *chunker.Chunker, sh *snapshot, data []byte) {
	assert := assert.New(t)
	// Successful read but to much
	rc, err := ch.GetReader(ctx, sh.ID, 0)
	assert.NoError(err)
	read := make([]byte, len(data)+1)
	nn, err := io.ReadFull(rc, read)
	assert.EqualError(err, io.ErrUnexpectedEOF.Error())
	assert.Equal(len(data), nn)
	assert.Equal(0, bytes.Compare(data, read[:len(data)]), "data read differs from written")
	if nn-len(data) > 0 {
		assert.Equal(0, bytes.Compare(data[:nn-len(data)], read[len(data):nn]), "data read differs from written")
	}
	assert.NoError(rc.Close())
}

func get(ctx context.Context, t *testing.T, st storage.Storage, sh *snapshot) {
	assert := assert.New(t)
	tmp, err := st.GetSnapshot(ctx, sh.ID)
	require.NoError(t, err)
	assert.Equal(sh.ID, tmp.ID, "id differs")
	assert.Equal(sh.Base, tmp.Base, "base differs")
	assert.Equal(sh.Size, tmp.Size, "size differs")
	assert.Equal(sh.Metadata, tmp.Metadata, "metadata differs")
	assert.Equal(sh.Organization, tmp.Organization, "organization differs")
	assert.Equal(sh.ProjectID, tmp.ProjectID, "project differs")
	assert.Equal(sh.Disk, tmp.Disk, "disk differs")
	assert.Equal(sh.Public, tmp.Public, "public differs")
	assert.Equal(sh.Name, tmp.Name, "name differs")
	assert.Equal(sh.Description, tmp.Description, "description differs")
	assert.Equal(sh.ProductID, tmp.ProductID, "product_id differs")
	assert.Equal(sh.ImageID, tmp.ImageID, "image_id differs")
	sh.SnapshotInfo = *tmp
}

func readNotReady(ctx context.Context, t *testing.T, ch *chunker.Chunker, sh *snapshot) {
	assert := assert.New(t)
	_, err := ch.GetReader(ctx, sh.ID, 0)
	assert.EqualError(err, misc.ErrSnapshotNotReady.Error())
}

func write(ctx context.Context, t *testing.T, ch *chunker.Chunker, sh *snapshot, data []byte, offset int64) {
	assert := assert.New(t)
	strctx := chunker.StreamContext{
		Reader:    bytes.NewReader(data),
		ChunkSize: chunkSize,
		Offset:    offset,
	}
	nn, err := ch.StoreFromStream(ctx, sh.ID, strctx)
	if len(data)%int(chunkSize) != 0 {
		assert.EqualError(err, misc.ErrSmallChunk.Error())
	} else {
		assert.NoError(err, "io.EOF must not be returned")
	}
	assert.Equal(len(data)/int(chunkSize), nn, "invalid chunks count")
}

func writeBad(ctx context.Context, t *testing.T, ch *chunker.Chunker, sh *snapshot, data []byte, _ int64) {
	assert := assert.New(t)
	strctx := chunker.StreamContext{
		Reader: bytes.NewReader(data),
	}
	strctx.ChunkSize = 1000 * chunkSize
	_, err := ch.StoreFromStream(ctx, sh.ID, strctx)
	assert.EqualError(err, misc.ErrChunkSizeTooBig.Error())

	strctx.ChunkSize = chunkSize
	strctx.Offset = 1
	_, err = ch.StoreFromStream(ctx, sh.ID, strctx)
	assert.EqualError(err, misc.ErrInvalidOffset.Error())
}

func writeReadOnly(ctx context.Context, t *testing.T, ch *chunker.Chunker, st storage.Storage, sh *snapshot, data []byte, offset int64) {
	assert := assert.New(t)
	strctx := chunker.StreamContext{
		Reader:    bytes.NewReader(data),
		ChunkSize: chunkSize,
		Offset:    offset,
	}
	_, err := ch.StoreFromStream(ctx, sh.ID, strctx)
	assert.EqualError(err, misc.ErrSnapshotReadOnly.Error())
	err = st.EndSnapshot(ctx, sh.ID)
	assert.EqualError(err, misc.ErrSnapshotReadOnly.Error())
}

func cycle(ctx context.Context, t *testing.T, ch *chunker.Chunker, st storage.Storage, sh *snapshot, data []byte, toWrite int) {
	var err error
	sh.ID, err = st.BeginSnapshot(ctx, &sh.CreationInfo)
	require.NoError(t, err, "Storing snapshot failed")

	write(ctx, t, ch, sh, data[:toWrite], 0)

	err = st.EndSnapshot(ctx, sh.ID)
	require.NoError(t, err, "Storing snapshot failed")

	t.Run("Read", func(t *testing.T) {
		read(ctx, t, ch, sh, data)
		get(ctx, t, st, sh)
		require.Equal(t, int64(toWrite)/chunkSize*chunkSize, sh.RealSize)
	})
}

func cycleDiff(ctx context.Context, t *testing.T, data []byte, namePrefix string, reverseDelete bool, conf *config.Config) {
	st := getStorage(ctx, t, conf)
	defer st.Close()
	sh := snapshot{
		SnapshotInfo: common.SnapshotInfo{
			CreationInfo: common.CreationInfo{
				CreationMetadata: common.CreationMetadata{
					ID: makeUUID(t),
					UpdatableMetadata: common.UpdatableMetadata{
						Description: "parent",
					},
					Name: fmt.Sprintf("%s_%s", namePrefix, "parent"),
				},
				Size: int64(len(data)),
			},
		},
	}
	ch := chunker.NewChunker(st, chunker.MustBuildHasher(conf.General.ChecksumAlgorithm), chunker.NewGZipCompressor)
	cycle(ctx, t, ch, st, &sh, data, len(data))

	child := snapshot{
		SnapshotInfo: common.SnapshotInfo{
			CreationInfo: common.CreationInfo{
				CreationMetadata: common.CreationMetadata{
					ID: makeUUID(t),
					UpdatableMetadata: common.UpdatableMetadata{
						Description: "child",
					},
					Name: fmt.Sprintf("%s_%s", namePrefix, "child"),
				},
				Base: sh.ID,
				Size: int64(len(data)),
			},
		},
	}
	ch = chunker.NewChunker(st, chunker.MustBuildHasher(conf.General.ChecksumAlgorithm), chunker.NewLZ4Compressor)
	cycle(ctx, t, ch, st, &child, data, len(data)/2)

	bases, err := st.ListBases(ctx, &storage.BaseListRequest{ID: child.ID})
	require.NoError(t, err)
	require.Equal(t, 2, len(bases))
	require.Equal(t, child.ID, bases[0].ID)
	require.Equal(t, sh.ID, bases[1].ID)

	assert := assert.New(t)

	if reverseDelete {
		log.Println("Deleting parent", sh.ID)
		d, c, err := getFacade(st).deleteSnapshot(ctx, &common.DeleteRequest{ID: sh.ID})
		assert.NoError(err)
		err = d.DoWork(d.InitTaskWorkContext(c))
		assert.NoError(err)
		read(ctx, t, ch, &child, data)
		child.Base = ""
		get(ctx, t, st, &child)
		assert.Equal("", child.Base, "Child base is unchanged")
		assert.Equal(int64(len(data)), child.RealSize, "RealSize is incorrect")
		assert.Equal(child.Changes[len(child.Changes)-1].RealSize, child.RealSize)

		// TODO: remove
		_ = st.Close()
		st = getStorage(ctx, t, conf)
		defer st.Close()
		cleared, err := st.IsSnapshotCleared(ctx, sh.ID)
		assert.NoError(err)
		assert.True(cleared)
	}

	log.Println("Deleting child", child.ID)
	d, c, err := getFacade(st).deleteSnapshot(ctx, &common.DeleteRequest{ID: child.ID})
	assert.NoError(err)
	err = d.DoWork(d.InitTaskWorkContext(c))
	assert.NoError(err)

	if !reverseDelete {
		// TODO: remove
		_ = st.Close()
		st = getStorage(ctx, t, conf)
		defer st.Close()
		cleared, err := st.IsSnapshotCleared(ctx, child.ID)
		assert.NoError(err)
		assert.True(cleared)

		ch := chunker.NewChunker(st, chunker.MustBuildHasher(conf.General.ChecksumAlgorithm), chunker.NewLZ4Compressor)
		read(ctx, t, ch, &sh, data)
		log.Println("Deleting parent", sh.ID)
		err = getFacade(st).DeleteSnapshot(ctx, &common.DeleteRequest{ID: sh.ID})
		assert.NoError(err)
	}
}

func testFunctional(ctx context.Context, t *testing.T, st storage.Storage) {
	assert := assert.New(t)
	data := make([]byte, 8*chunkSize)
	rand.Read(data[len(data)/2:])
	sh := snapshot{
		SnapshotInfo: common.SnapshotInfo{
			CreationInfo: common.CreationInfo{
				CreationMetadata: common.CreationMetadata{
					UpdatableMetadata: common.UpdatableMetadata{
						Metadata:    META,
						Description: DESC,
					},
					ID:           makeUUID(t),
					Organization: ORG,
					ProjectID:    PROJECT,
					ProductID:    productID,
					ImageID:      imageID,
					Name:         NAME,
				},
				Disk: DISK,
			},
		},
	}
	conf, e := config.GetConfig()
	require.NoError(t, e)

	ch := chunker.NewChunker(st, chunker.MustBuildHasher(conf.General.ChecksumAlgorithm), chunker.NewLZ4Compressor)
	_, err := st.BeginSnapshot(ctx, &sh.CreationInfo)
	assert.EqualError(err, misc.ErrInvalidSize.Error())
	sh.Size = int64(len(data))

	sh.Name = ""
	_, err = st.BeginSnapshot(ctx, &sh.CreationInfo)
	assert.EqualError(err, misc.ErrInvalidName.Error())
	sh.Name = NAME

	sh.Base = "fdsfdsfdfdsf"
	_, err = st.BeginSnapshot(ctx, &sh.CreationInfo)
	assert.EqualError(err, misc.ErrSnapshotNotFound.Error())

	sh.Base = ""
	id, err := st.BeginSnapshot(ctx, &sh.CreationInfo)
	require.NoError(t, err)
	assert.Equal(sh.ID, id)

	sh.ID = ""
	_, err = st.BeginSnapshot(ctx, &sh.CreationInfo)
	assert.EqualError(err, misc.ErrDuplicateName.Error())
	sh.ID = id

	sh.Name = makeUUID(t)
	_, err = st.BeginSnapshot(ctx, &sh.CreationInfo)
	assert.EqualError(err, misc.ErrDuplicateID.Error())
	sh.Name = NAME

	t.Run("parallel", func(t *testing.T) {
		t.Run("WriteBad", func(t *testing.T) {
			t.Parallel()
			writeBad(ctx, t, ch, &sh, data[:len(data)/2], 0)
		})
		t.Run("WriteOtherHalf", func(t *testing.T) {
			t.Parallel()
			write(ctx, t, ch, &sh, data[len(data)/2:len(data)-1], int64(len(data)/2))
		})
	})

	write(ctx, t, ch, &sh, data[len(data)-int(chunkSize):], int64(len(data))-chunkSize)
	t.Log(sh.ID, sh.ProjectID)

	t.Run("parallel", func(t *testing.T) {
		t.Run("GetUncommitted", func(t *testing.T) {
			t.Parallel()
			get(ctx, t, st, &sh)
		})
		t.Run("ReadUncommitted", func(t *testing.T) {
			t.Parallel()
			readNotReady(ctx, t, ch, &sh)
		})
	})

	err = st.EndSnapshot(ctx, sh.ID)
	assert.NoError(err)

	t.Run("parallel", func(t *testing.T) {
		t.Run("WriteReadOnly", func(t *testing.T) {
			t.Parallel()
			writeReadOnly(ctx, t, ch, st, &sh, data, 0)
		})
		t.Run("Get", func(t *testing.T) {
			t.Parallel()
			get(ctx, t, st, &sh)
		})
		t.Run("ReadBad", func(t *testing.T) {
			t.Parallel()
			readBad(ctx, t, ch, &sh, data)
		})
		t.Run("Read", func(t *testing.T) {
			t.Parallel()
			read(ctx, t, ch, &sh, data)
		})
	})

	err = getFacade(st).DeleteSnapshot(ctx, &common.DeleteRequest{ID: sh.ID})
	assert.NoError(err)

	t.Run("parallel", func(t *testing.T) {
		t.Run("LoadDeleted", func(t *testing.T) {
			t.Parallel()
			_, err = st.GetLiveSnapshot(ctx, sh.ID)
			assert.EqualError(err, misc.ErrSnapshotNotFound.Error())
		})
		t.Run("DeleteDeleted", func(t *testing.T) {
			t.Parallel()
			err = getFacade(st).DeleteSnapshot(ctx, &common.DeleteRequest{ID: sh.ID})
			assert.EqualError(err, misc.ErrSnapshotNotFound.Error())
		})
	})
}

func getStorage(ctx context.Context, t *testing.T, conf *config.Config) storage.Storage {
	if conf == nil {
		c, err := config.GetConfig()
		require.NoError(t, err)
		conf = &c
	}

	st, err := PrepareStorage(ctx, conf, misc.TableOpNone)
	require.NoError(t, err)
	return st
}

func getFacade(st storage.Storage) *Facade {
	return &Facade{
		st:  st,
		pr:  pr,
		mcf: &tasks.MoveContextFactory{Proxy: pr},
		tm:  tasks.NewTaskManager(taskLimiterStub{}, ""),
	}
}

func TestConfig(t *testing.T) {
	cfg, err := config.GetConfig()
	ar := require.New(t)
	ar.NoError(err)
	ar.NotEmpty(cfg.General.ZoneID)
	ar.NotEmpty(cfg.TaskLimiter.CheckInterval)
	ar.NotEmpty(cfg.TaskLimiter.ZoneTotal)
}

func TestStoreFromStream(t *testing.T) {
	ctx, ctxCancel := createTestContext(t)
	defer ctxCancel()

	st := getStorage(ctx, t, nil)
	defer st.Close()
	testFunctional(ctx, t, st)
}

// TODO: Need test for random tree generation and realsize calculation check.

func TestStoreFromStreamDifferential(t *testing.T) {
	ctx, ctxCancel := createTestContext(t)
	defer ctxCancel()

	conf, err := config.GetConfig()
	require.NoError(t, err)
	data1 := make([]byte, 8*chunkSize)
	rand.Read(data1)
	data2 := make([]byte, 8*chunkSize)
	rand.Read(data2)

	t.Run("parallel", func(t *testing.T) {
		t.Run("DiffDeleteChild", func(t *testing.T) {
			t.Parallel()
			cycleDiff(ctx, t, data1, "child", false, &conf)
		})
		t.Run("DiffDeleteParent", func(t *testing.T) {
			t.Parallel()
			cycleDiff(ctx, t, data2, "parent", true, &conf)
		})
	})
}

func TestSizeChanges(t *testing.T) {
	ctx, ctxCancel := createTestContext(t)
	defer ctxCancel()

	st := getStorage(ctx, t, nil)
	defer st.Close()

	conf, e := config.GetConfig()
	require.NoError(t, e)

	ch := chunker.NewChunker(st, chunker.MustBuildHasher(conf.General.ChecksumAlgorithm), chunker.NewLZ4Compressor)

	data := make([]byte, chunkSize)
	rand.Read(data)

	var ci common.CreationInfo
	ci.Size = 8 * chunkSize
	snapshots := []struct {
		ID       string
		Blocks   []int
		FullSize int64
	}{
		{"", []int{0}, 1 * chunkSize},
		{"", []int{0, 1}, 2 * chunkSize},
		{"", []int{0, 1, 2, 3}, 4 * chunkSize},
		{"", []int{0, 1, 4, 5}, 6 * chunkSize},
		{"", []int{0, 2, 4, 6}, 7 * chunkSize},
	}
	// Test chain merge
	delOrder := []int{0, 2, 1, 3}

	var err error
	for i, sh := range snapshots {
		ci.Name = fmt.Sprintf("child_%d", i)
		snapshots[i].ID = makeUUID(t)
		ci.ID = snapshots[i].ID
		ctxSnapshot := ctxlog.WithLogger(ctx, ctxlog.G(ctx).With(logging.SnapshotID(snapshots[i].ID)))
		if i > 0 {
			ci.Base = snapshots[i-1].ID
		}
		_, err = st.BeginSnapshot(ctxSnapshot, &ci)
		require.NoError(t, err)
		for _, index := range sh.Blocks {
			err = ch.StoreChunk(ctxSnapshot, snapshots[i].ID, &chunker.StreamChunk{Offset: int64(index) * chunkSize, Data: data})
			require.NoError(t, err)
		}
		err = st.EndSnapshot(ctxSnapshot, snapshots[i].ID)
		require.NoError(t, err)
	}

	// ready -> deleting
	for _, index := range delOrder {
		ctxSnapshot := ctxlog.WithLogger(ctx, ctxlog.G(ctx).With(logging.SnapshotID(snapshots[index].ID)))
		err = st.BeginDeleteSnapshot(ctxSnapshot, snapshots[index].ID)
		require.NoError(t, err)
	}

	// deleting -> deleted
	var wg sync.WaitGroup
	wg.Add(len(snapshots) - 1)
	for _, sh := range snapshots[:len(snapshots)-1] {
		go func(id string) {
			defer wg.Done()
			var err error
			ctxSnapshot := ctxlog.WithLogger(ctx, ctxlog.G(ctx).With(logging.SnapshotID(id)))
			for {
				err = st.EndDeleteSnapshotSync(ctxSnapshot, id, false)
				if err != misc.ErrAlreadyLocked {
					break
				}
				time.Sleep(time.Millisecond * time.Duration(rand.Int31n(100)))
			}
			require.NoError(t, err)
		}(sh.ID)
	}
	wg.Wait()

	ctxlog.G(ctx).Info("Snapshots delete finished.")

	info, err := st.GetSnapshot(ctx, snapshots[3].ID)
	require.NoError(t, err)
	require.Equal(t, 3, len(info.Changes))
	require.Equal(t, info.RealSize, info.Changes[len(info.Changes)-1].RealSize)
	require.Equal(t, snapshots[3].FullSize, info.Changes[len(info.Changes)-2].RealSize)

	info, err = st.GetSnapshot(ctx, snapshots[4].ID)
	require.NoError(t, err)
	require.Equal(t, snapshots[4].FullSize, info.RealSize)
	require.Equal(t, info.RealSize, info.Changes[len(info.Changes)-1].RealSize)

	err = getFacade(st).DeleteSnapshot(ctx, &common.DeleteRequest{ID: snapshots[4].ID})
	require.NoError(t, err)
}

func TestStoreFromStreamParallel(t *testing.T) {
	ctx, ctxCancel := createTestContext(t)
	defer ctxCancel()

	st := getStorage(ctx, t, nil)
	defer st.Close()
	var wg sync.WaitGroup
	for i := 0; i < 4; i++ {
		log.Printf("Starting goroutine #%v\n", i)
		wg.Add(1)
		go func(i int) {
			defer wg.Done()
			log.Printf("Executing goroutine #%v\n", i)
			sh := snapshot{
				SnapshotInfo: common.SnapshotInfo{
					CreationInfo: common.CreationInfo{
						CreationMetadata: common.CreationMetadata{
							ID: makeUUID(t),
							UpdatableMetadata: common.UpdatableMetadata{
								Description: DESC,
							},
							Name: fmt.Sprintf("%s_%d", NAME, i),
						},
						Size: 4 * chunkSize,
					},
				},
			}
			conf, e := config.GetConfig()
			require.NoError(t, e)

			ch := chunker.NewChunker(st, chunker.MustBuildHasher(conf.General.ChecksumAlgorithm), chunker.NewLZ4Compressor)
			var err error
			_, err = st.BeginSnapshot(ctx, &sh.CreationInfo)
			require.NoError(t, err)

			data := make([]byte, 4*chunkSize)
			strctx := chunker.StreamContext{
				Reader:    bytes.NewReader(data),
				ChunkSize: chunkSize,
				Offset:    0,
			}
			_, err = ch.StoreFromStream(ctx, sh.ID, strctx)
			require.NoError(t, err)

			err = st.EndSnapshot(ctx, sh.ID)
			require.NoError(t, err)

			err = getFacade(st).DeleteSnapshot(ctx, &common.DeleteRequest{ID: sh.ID})
			require.NoError(t, err)

			log.Printf("Finished goroutine #%v\n", i)
		}(i)
	}
	wg.Wait()
}

func list(ctx context.Context, t *testing.T, st storage.Storage, r *storage.ListRequest) []*snapshot {
	assert := assert.New(t)
	infos, err := st.List(ctx, r)
	assert.NoError(err)
	snapshots := make([]*snapshot, 0, len(infos))
	for _, info := range infos {
		snapshots = append(snapshots, &snapshot{
			SnapshotInfo: info,
		})
	}
	return snapshots
}

func difference(l1 []*snapshot, l2 []*snapshot) []*snapshot {
	m := make(map[string]bool)
	for _, i := range l2 {
		m[i.ID] = true
	}

	var r []*snapshot
	for _, i := range l1 {
		_, ok := m[i.ID]
		if !ok {
			r = append(r, i)
		}
	}
	return r
}

func TestList(t *testing.T) {
	ctx, ctxCancel := createTestContext(t)
	defer ctxCancel()

	st := getStorage(ctx, t, nil)
	defer st.Close()

	tim := time.Now()
	var withBilling []*snapshot
	t.Run("ListAll", func(t *testing.T) {
		r := &storage.ListRequest{
			BillingEnd: &tim,
		}
		withBilling = list(ctx, t, st, r)
	})
	if len(withBilling) == 0 {
		t.Skip("No snapshots in DB")
	}
	count := len(withBilling)

	t.Run("parallel", func(t *testing.T) {
		t.Run("ListDisk", func(t *testing.T) {
			t.Parallel()
			r := &storage.ListRequest{
				BillingEnd: &tim,
				Disk:       DISK,
			}
			snapshots := list(ctx, t, st, r)
			for _, sh := range snapshots {
				assert.Equal(t, r.Disk, sh.Disk)
			}
		})
		t.Run("ListProject", func(t *testing.T) {
			t.Parallel()
			r := &storage.ListRequest{
				BillingEnd: &tim,
				Project:    PROJECT,
			}
			snapshots := list(ctx, t, st, r)
			for _, sh := range snapshots {
				assert.Equal(t, r.Project, sh.ProjectID)
			}
		})
		t.Run("ListN", func(t *testing.T) {
			t.Parallel()
			r := &storage.ListRequest{
				BillingEnd: &tim,
				N:          1,
			}
			snapshots := list(ctx, t, st, r)
			assert.Equal(t, r.N, int64(len(snapshots)))
		})
		t.Run("ListLast", func(t *testing.T) {
			t.Parallel()
			r := &storage.ListRequest{
				BillingEnd: &tim,
				Last:       withBilling[0].ID,
			}
			snapshots := list(ctx, t, st, r)
			assert.Equal(t, count-1, len(snapshots))
		})
		t.Run("ListBilling", func(t *testing.T) {
			t.Parallel()
			// Must not return snapshots deleted before BillingStart
			alive := list(ctx, t, st, &storage.ListRequest{})
			snapshots := difference(withBilling, alive)

			var lastDeath time.Time
			for _, sh := range snapshots {
				assert.Equal(t, int64(0), sh.RealSize)
				assert.Equal(t, int64(0), sh.Changes[len(sh.Changes)-1].RealSize)
				if sh.Changes[len(sh.Changes)-1].Timestamp.After(lastDeath) {
					lastDeath = sh.Changes[len(sh.Changes)-1].Timestamp
				}
			}

			start := lastDeath.Add(time.Microsecond)
			r := &storage.ListRequest{
				BillingEnd:   &tim,
				BillingStart: &start,
			}
			snapshots = list(ctx, t, st, r)

			// Creating and failed snapshots are useless for billing
			var aliveBilling []*snapshot
			for _, sh := range alive {
				if sh.State.Code != storage.StateCreating && sh.State.Code != storage.StateFailed {
					aliveBilling = append(aliveBilling, sh)
				}
			}
			assert.Equal(t, len(aliveBilling), len(snapshots))
			assert.Empty(t, difference(snapshots, aliveBilling))
		})
		t.Run("ListSortName", func(t *testing.T) {
			t.Parallel()
			r := &storage.ListRequest{
				BillingEnd: &tim,
				Sort:       "name",
			}
			snapshots := list(ctx, t, st, r)
			for i, sh := range snapshots {
				if i > 0 {
					if !assert.True(t, sh.Name >= snapshots[i-1].Name) {
						t.Log(sh.Name, snapshots[i-1].Name)
					}
				}
			}
		})
		t.Run("ListSortName-", func(t *testing.T) {
			t.Parallel()
			r := &storage.ListRequest{
				BillingEnd: &tim,
				Sort:       "-name",
			}
			snapshots := list(ctx, t, st, r)
			for i, sh := range snapshots {
				if i > 0 {
					assert.True(t, sh.Name <= snapshots[i-1].Name)
				}
			}
		})
		t.Run("ListSortDescription", func(t *testing.T) {
			t.Parallel()
			r := &storage.ListRequest{
				BillingEnd: &tim,
				Sort:       "description",
			}
			snapshots := list(ctx, t, st, r)
			for i, sh := range snapshots {
				if i > 0 {
					if !assert.True(t, sh.Description >= snapshots[i-1].Description) {
						t.Log(sh.Description, snapshots[i-1].Description)
					}
				}
			}
		})
		t.Run("ListSortDescription-", func(t *testing.T) {
			t.Parallel()
			r := &storage.ListRequest{
				BillingEnd: &tim,
				Sort:       "-description",
			}
			snapshots := list(ctx, t, st, r)
			for i, sh := range snapshots {
				if i > 0 {
					assert.True(t, sh.Description <= snapshots[i-1].Description)
				}
			}
		})
		t.Run("ListSortCreated", func(t *testing.T) {
			t.Parallel()
			r := &storage.ListRequest{
				BillingEnd: &tim,
				Sort:       "created",
			}
			snapshots := list(ctx, t, st, r)
			for i, sh := range snapshots {
				if i > 0 {
					assert.False(t, sh.Created.Before(snapshots[i-1].Created))
				}
			}
		})
		t.Run("ListSortCreated-", func(t *testing.T) {
			t.Parallel()
			r := &storage.ListRequest{
				BillingEnd: &tim,
				Sort:       "-created",
			}
			snapshots := list(ctx, t, st, r)
			for i, sh := range snapshots {
				if i > 0 {
					assert.False(t, sh.Created.After(snapshots[i-1].Created))
				}
			}
		})
		t.Run("ListSortRealSize", func(t *testing.T) {
			t.Parallel()
			r := &storage.ListRequest{
				BillingEnd: &tim,
				Sort:       "real_size",
			}
			snapshots := list(ctx, t, st, r)
			for i, sh := range snapshots {
				if i > 0 {
					assert.True(t, sh.RealSize >= snapshots[i-1].RealSize)
				}
			}
		})
		t.Run("ListSortRealSize-", func(t *testing.T) {
			t.Parallel()
			r := &storage.ListRequest{
				BillingEnd: &tim,
				Sort:       "-real_size",
			}
			snapshots := list(ctx, t, st, r)
			for i, sh := range snapshots {
				if i > 0 {
					assert.True(t, sh.RealSize <= snapshots[i-1].RealSize)
				}
			}
		})
	})
}

func TestConvert(t *testing.T) {
	ctx, ctxCancel := createTestContext(t)
	defer ctxCancel()

	conf, err := config.GetConfig()
	require.NoError(t, err)

	conf.Performance.ConvertWorkers = 1
	conf.S3.Dummy = true
	conf.General.DummyFsRoot, err = ioutil.TempDir("", "snapshot-test")
	require.NoError(t, err)
	defer os.RemoveAll(conf.General.DummyFsRoot)

	tmpDir, err := ioutil.TempDir("", "")
	if err != nil {
		t.Fatal("create tmp dir", err)
	}
	conf.QemuDockerProxy.SocketPath = filepath.Join(tmpDir, "proxy.sock")
	conf.QemuDockerProxy.HostForDocker = "127.0.0.1:1234"

	f, err := NewFacadeOps(ctx, &conf, misc.TableOpNone)
	require.NoError(t, err)
	defer f.Close(ctx)
	defer ctxlog.G(ctx).Debug("Defer close Facade")

	d := common.CreationMetadata{
		ProjectID: PROJECT,
		ImageID:   imageID,
	}
	r := common.ConvertRequest{
		CreationMetadata: d,
		URL:              convertURL,
	}
	sh := snapshot{
		SnapshotInfo: common.SnapshotInfo{
			CreationInfo: common.CreationInfo{
				CreationMetadata: d,
				Size:             10 * chunkSize, // we know its size
			},
		},
	}

	testSuccessConvert := func(t *testing.T, name, url string) {
		if runtime.GOOS == darwin || runtime.GOOS == windows {
			t.Skipf("Can't run convert test on this OS because can't mount qemu-nbd: '%v'", runtime.GOOS)
		}
		ctxlog.G(ctx).Debug("Start test convert", zap.String("name", name), zap.String("url", url))

		tmp := r
		tmp.Name = name
		tmp.URL = url
		sh.Name = tmp.Name
		var taskID string
		ctxlog.G(ctx).Debug("Test success convert request", zap.Any("request", tmp))
		sh.ID, taskID, err = f.ConvertSnapshot(ctx, &tmp)
		require.NoError(t, err)
		defer f.DeleteSnapshot(ctx, &common.DeleteRequest{ID: sh.ID})

		// wait
		convertDeadline := time.Now().Add(time.Minute * 5)
		for time.Now().Before(convertDeadline) {
			status, err := f.GetTaskStatus(ctx, taskID)
			require.NoError(t, err)
			require.NoError(t, status.Error)
			if status.Finished {
				break
			}
			time.Sleep(time.Second)
		}

		info, err := f.st.GetSnapshot(ctx, sh.ID)
		require.NoError(t, err)
		sh.SnapshotInfo = *info
		switch sh.State.Code {
		case storage.StateReady:
		case storage.StateCreating:
			t.Fatal(fmt.Errorf("convert timeout"))
		case storage.StateFailed:
			t.Fatal(fmt.Errorf("convert failed"))
		default:
			t.Fatal(fmt.Errorf("invalid state"))
		}

		get(ctx, t, f.st, &sh)
	}

	t.Run("Success-nbd", func(t *testing.T) {
		testSuccessConvert(t, "ConvertSuccessNBD", convertURL)
	})

	t.Run("Success-direct", func(t *testing.T) {
		testSuccessConvert(t, "ConvertSuccessDirect", rawImageURL)
	})

	t.Run("Fail", func(t *testing.T) {
		assert := assert.New(t)
		// force status update
		storage.UpdateProgressProbability = 1.0

		r.Name = "ConvertFail"
		sh.Name = r.Name
		tmp := r
		var taskID string
		sh.ID, taskID, err = f.ConvertSnapshot(ctx, &tmp)
		require.NoError(t, err)
		defer f.DeleteSnapshot(ctx, &common.DeleteRequest{ID: sh.ID})

		var intr bool

		// test modify proxy response for simulate errors
		oldModifyResponse := f.pr.ModifyResponse
		defer func() {
			ctxlog.G(ctx).Info("Proxy restore")

			//restore normal proxy function
			f.pr.ModifyResponse = oldModifyResponse
		}()

		// wait
		retryDeadline := time.Now().Add(time.Minute * 5)
		for time.Now().Before(retryDeadline) {
			status, err := f.GetTaskStatus(ctx, taskID)
			require.NoError(t, err)
			if status.Offset > 0 && !intr {
				ctxlog.G(ctx).Info("Proxy interrupt")

				// Interrupt
				// block any request through proxy
				f.pr.ModifyResponse = func(response *http.Response) error {
					response.StatusCode = http.StatusNotFound
					return nil
				}
				intr = true
			}
			if status.Finished {
				if status.Error == nil && intr {
					t.Fatal(fmt.Errorf("interrupt did not work"))
				}
				if status.Error != nil && !intr {
					t.Fatal(fmt.Errorf("failed without interrupt"))
				}
				break
			}
			time.Sleep(10 * time.Millisecond)
		}

		info, err := f.st.GetSnapshot(ctx, sh.ID)
		require.NoError(t, err)
		sh.SnapshotInfo = *info
		if sh.State.Code == storage.StateCreating {
			t.Error("convert timeout")
		}

		// Wait a little while chunks are being cleared
		var cleared bool
		for i := 0; i < 20; i++ {
			cleared, err = f.st.IsSnapshotCleared(ctx, sh.ID)
			assert.NoError(err)
			if cleared {
				break
			}
			time.Sleep(time.Second)
		}
		// The must be no chunks
		assert.True(cleared)
	})

	t.Run("Redirect", func(t *testing.T) {
		r.Name = "ConvertRedirect"
		r.URL = redirectedURL
		sh.Name = r.Name
		tmp := r
		var taskID string
		sh.ID, taskID, err = f.ConvertSnapshot(ctx, &tmp)

		// wait
		for i := 0; i < 200; i++ {
			status, err := f.GetTaskStatus(ctx, taskID)
			require.NoError(t, err)
			if status.Finished {
				require.True(t, errors.Is(status.Error, misc.ErrDenyRedirect))
				return
			}
			require.NoError(t, status.Error)
			time.Sleep(100 * time.Millisecond)
		}
		t.Fatal(fmt.Errorf("convert timeout"))
	})
}

func copySnapshot(sh *snapshot) *snapshot {
	return &snapshot{
		SnapshotInfo: sh.SnapshotInfo,
	}
}

func TestGC(t *testing.T) {
	ctx, ctxCancel := createTestContext(t)
	defer ctxCancel()

	data := make([]byte, 8*chunkSize)
	rand.Read(data[len(data)/2:])

	conf, err := config.GetConfig()
	require.NoError(t, err)

	conf.GC.Enabled = true
	// 1 ns
	conf.GC.FailedCreation.Duration = 1
	conf.GC.FailedConversion.Duration = 1
	conf.GC.FailedDeletion.Duration = 1

	st := getStorage(ctx, t, &conf)
	defer st.Close()
	sh := snapshot{
		SnapshotInfo: common.SnapshotInfo{
			CreationInfo: common.CreationInfo{
				CreationMetadata: common.CreationMetadata{
					UpdatableMetadata: common.UpdatableMetadata{
						Metadata:    META,
						Description: DESC,
					},
					Organization: ORG,
					ProjectID:    PROJECT,
					ProductID:    productID,
					ImageID:      imageID,
					Name:         NAME,
				},
				Disk: DISK,
				Size: int64(len(data)),
			},
		},
	}
	ch := chunker.NewChunker(st, chunker.MustBuildHasher(conf.General.ChecksumAlgorithm), chunker.NewLZ4Compressor)

	// Create bad snapshots
	mtx := sync.Mutex{}
	ids := make(map[string]empty)
	t.Run("parallel", func(t *testing.T) {
		t.Run("Creating", func(t *testing.T) {
			t.Parallel()
			sh := copySnapshot(&sh)
			sh.Name = "Creating" + NAME
			sh.ID = makeUUID(t)
			_, err = st.BeginSnapshot(ctx, &sh.CreationInfo)
			require.NoError(t, err)
			write(ctx, t, ch, sh, data, 0)
			mtx.Lock()
			ids[sh.ID] = empty{}
			mtx.Unlock()
		})
		t.Run("Failed", func(t *testing.T) {
			t.Parallel()
			sh := copySnapshot(&sh)
			sh.Name = "Failed" + NAME
			sh.ID = makeUUID(t)
			_, err = st.BeginSnapshot(ctx, &sh.CreationInfo)
			require.NoError(t, err)

			write(ctx, t, ch, sh, data, 0)

			mtx.Lock()
			ids[sh.ID] = empty{}
			mtx.Unlock()

			err = st.UpdateSnapshotStatus(ctx, sh.ID, storage.StatusUpdate{
				State: storage.StateFailed,
				Desc:  "something was wrong",
			})
			require.NoError(t, err)
		})
		t.Run("Deleting", func(t *testing.T) {
			t.Parallel()
			sh := copySnapshot(&sh)
			sh.Name = "Deleting" + NAME
			sh.ID = makeUUID(t)
			_, err = st.BeginSnapshot(ctx, &sh.CreationInfo)
			require.NoError(t, err)

			write(ctx, t, ch, sh, data, 0)

			mtx.Lock()
			ids[sh.ID] = empty{}
			mtx.Unlock()

			err = st.BeginDeleteSnapshot(ctx, sh.ID)
			require.NoError(t, err)
		})
	})

	if !t.Run("gc", func(t *testing.T) {
		// Run GC
		gc, err := NewGC(ctx, &conf)
		require.NoError(t, err)

		err = gc.Once(ctx)
		require.NoError(t, err)
	}) {
		t.Fatal(err)
	}

	// Dump data
	snapshots := make(map[string]common.SnapshotInfo)

	var l1, l2 []*snapshot
	t.Run("parallel", func(t *testing.T) {
		t.Run("list1", func(t *testing.T) {
			t.Parallel()
			tim := time.Now()
			l1 = list(ctx, t, st, &storage.ListRequest{
				BillingEnd: &tim,
			})
		})
		t.Run("list1", func(t *testing.T) {
			t.Parallel()
			l2 = list(ctx, t, st, &storage.ListRequest{})
		})
	})

	for _, i := range l1 {
		snapshots[i.ID] = i.SnapshotInfo
	}
	for _, i := range l2 {
		snapshots[i.ID] = i.SnapshotInfo
	}

	// Check than snapshots are really deleted
	t.Run("parallel", func(t *testing.T) {
		for id := range ids {
			func(id string) {
				t.Run("check", func(t *testing.T) {
					t.Parallel()
					i, ok := snapshots[id]
					require.True(t, ok, "snapshot is deleted from DB")
					require.Equal(t, storage.StateDeleted, i.State.Code, "snapshot is not deleted")

					cleared, err := st.IsSnapshotCleared(ctx, id)
					require.NoError(t, err)
					require.True(t, cleared, "snapshot cleared")
				})
			}(id)
		}
	})

	conf.GC.Tombstone.Duration = 1
	if !t.Run("gc", func(t *testing.T) {
		// Run GC
		gc, err := NewGC(ctx, &conf)
		require.NoError(t, err)

		err = gc.Once(ctx)
		require.NoError(t, err)
	}) {
		t.Fatal(err)
	}

	// Check than tombstones are deleted
	t.Run("parallel", func(t *testing.T) {
		for id := range ids {
			func(id string) {
				t.Run("check", func(t *testing.T) {
					t.Parallel()
					_, err := st.GetSnapshot(ctx, id)
					require.EqualError(t, err, misc.ErrSnapshotNotFound.Error())
				})
			}(id)
		}
	})
}

func TestGCHierarchy(t *testing.T) {
	ctx, ctxCancel := createTestContext(t)
	defer ctxCancel()

	conf, err := config.GetConfig()
	require.NoError(t, err)

	st := getStorage(ctx, t, &conf)
	defer st.Close()

	ch := chunker.NewChunker(st, chunker.MustBuildHasher(conf.General.ChecksumAlgorithm), chunker.NewGZipCompressor)

	data := make([]byte, chunkSize)
	rand.Read(data)

	suffix := makeUUID(t)

	var ci common.CreationInfo
	ci.Size = 8 * chunkSize
	snapshots := []struct {
		ID       string
		Blocks   []int
		FullSize int64
	}{
		{"", []int{0}, 1 * chunkSize},
		{"", []int{0, 1}, 2 * chunkSize},
		{"", []int{0, 1, 2, 3}, 4 * chunkSize},
		{"", []int{0, 1, 4, 5}, 6 * chunkSize},
		{"", []int{0, 2, 4, 6}, 7 * chunkSize},
	}
	// Test chain merge
	delOrder := []int{0, 2, 1, 3, 4}

	if !t.Run("setup", func(t *testing.T) {
		for i, sh := range snapshots {
			ci.Name = fmt.Sprintf("child_%d_%s", i, suffix)
			snapshots[i].ID = makeUUID(t)
			ci.ID = snapshots[i].ID
			if i > 0 {
				ci.Base = snapshots[i-1].ID
			}
			_, err = st.BeginSnapshot(ctx, &ci)
			require.NoError(t, err)
			for _, index := range sh.Blocks {
				err = ch.StoreChunk(ctx, snapshots[i].ID, &chunker.StreamChunk{Offset: int64(index) * chunkSize, Data: data})
				require.NoError(t, err)
			}
			err = st.EndSnapshot(ctx, snapshots[i].ID)
			require.NoError(t, err)
		}

		// ready -> deleting
		for _, index := range delOrder {
			err = st.BeginDeleteSnapshot(ctx, snapshots[index].ID)
			require.NoError(t, err)
		}
	}) {
		t.Fatal(err)
	}

	conf.GC.Enabled = true
	// 1 ns
	conf.GC.FailedCreation.Duration = 1
	conf.GC.FailedConversion.Duration = 1
	conf.GC.FailedDeletion.Duration = 1
	conf.GC.Tombstone.Duration = 1 * time.Hour

	gc, err := NewGC(ctx, &conf)
	require.NoError(t, err)

	require.True(t, t.Run("first GC", func(t *testing.T) {
		err = gc.Once(ctx)
		require.NoError(t, err)
	}))

	require.True(t, t.Run("check clear", func(t *testing.T) {
		for _, sh := range snapshots {
			cleared, err := st.IsSnapshotCleared(ctx, sh.ID)
			require.NoError(t, err)
			require.True(t, cleared, "snapshot cleared")
		}
	}))

	conf.GC.Tombstone.Duration = 1

	require.True(t, t.Run("second GC", func(t *testing.T) {
		err = gc.Once(ctx)
		require.NoError(t, err)
	}))

	require.True(t, t.Run("check tombstones", func(t *testing.T) {
		for _, sh := range snapshots {
			_, err := st.GetSnapshot(ctx, sh.ID)
			require.EqualError(t, err, misc.ErrSnapshotNotFound.Error())
		}
	}))
}

func TestCopy(t *testing.T) {
	ctx, ctxCancel := createTestContext(t)
	defer ctxCancel()

	st := getStorage(ctx, t, nil)
	defer st.Close()

	var err error
	suffix := makeUUID(t)

	conf, err := config.GetConfig()
	require.NoError(t, err)

	ch := chunker.NewChunker(st, chunker.MustBuildHasher(conf.General.ChecksumAlgorithm), chunker.NewGZipCompressor)

	data := make([]byte, chunkSize)
	rand.Read(data)

	var ci common.CreationInfo
	ci.Size = 8 * chunkSize
	snapshots := []struct {
		ID       string
		Name     string
		Blocks   []int
		FullSize int64
	}{
		{Blocks: []int{0}, FullSize: 1 * chunkSize},
		{Blocks: []int{0, 1}, FullSize: 2 * chunkSize},
		{Blocks: []int{0, 1, 2, 3}, FullSize: 4 * chunkSize},
		{Blocks: []int{0, 1, 4, 5}, FullSize: 6 * chunkSize},
		{Blocks: []int{0, 2, 4, 6}, FullSize: 7 * chunkSize},
	}
	// Test chain merge
	delOrder := []int{0, 2, 1, 3, 4}

	if !t.Run("setup", func(t *testing.T) {
		for i, sh := range snapshots {
			snapshots[i].Name = fmt.Sprintf("child_%d_%s", i, suffix)
			ci.Name = snapshots[i].Name
			snapshots[i].ID = makeUUID(t)
			ci.ID = snapshots[i].ID
			if i > 0 {
				ci.Base = snapshots[i-1].ID
			}
			_, err = st.BeginSnapshot(ctx, &ci)
			require.NoError(t, err)
			for _, index := range sh.Blocks {
				err = ch.StoreChunk(ctx, snapshots[i].ID, &chunker.StreamChunk{Offset: int64(index) * chunkSize, Data: data})
				require.NoError(t, err)
			}
			err = st.EndSnapshot(ctx, snapshots[i].ID)
			require.NoError(t, err)
		}
	}) {
		t.Fatal(err)
	}

	_, err = chunker.NewCalculateContext(st, snapshots[len(snapshots)-1].ID).UpdateChecksum(ctx)
	require.NoError(t, err)

	var id string
	if !t.Run("copy", func(t *testing.T) {
		facade := getFacade(st)
		taskID := makeUUID(t)
		id, err = facade.ShallowCopy(ctx, &common.CopyRequest{
			ID:              snapshots[len(snapshots)-1].ID,
			TargetProjectID: PROJECT,
			Name:            snapshots[len(snapshots)-1].Name,
			Params: common.CopyParams{
				TaskID: taskID,
			},
		})
		require.NoError(t, err)

		var status *common.TaskStatus
		for i := 0; i < 100; i++ {
			status, err = facade.GetTaskStatus(ctx, taskID)
			require.NoError(t, err)
			if status.Finished {
				require.NoError(t, status.Error)
				break
			}
			time.Sleep(100 * time.Millisecond)
		}
		require.True(t, status.Finished)
	}) {
		t.Fatal(err)
	}
	t.Logf("Copy ID: %s\n", id)

	if !t.Run("delete", func(t *testing.T) {
		// ready -> deleting
		for _, index := range delOrder {
			err = st.BeginDeleteSnapshot(ctx, snapshots[index].ID)
			require.NoError(t, err)
		}

		// deleting -> deleted
		var wg sync.WaitGroup
		wg.Add(len(snapshots))
		for _, sh := range snapshots {
			go func(id string) {
				defer wg.Done()
				var err error
				for i := 0; i < 300; i++ {
					err = st.EndDeleteSnapshotSync(ctxlog.WithLogger(ctx,
						ctxlog.GetLogger(ctx).With(logging.SnapshotID(id))), id, false)
					if err != misc.ErrAlreadyLocked {
						break
					}
					time.Sleep(100 * time.Millisecond)
				}
				require.NoError(t, err)
			}(sh.ID)
		}
		wg.Wait()
	}) {
		t.Fatal(err)
	}

	if !t.Run("check", func(t *testing.T) {
		sh, err := st.GetSnapshotFull(ctx, id)
		require.NoError(t, err)
		require.Equal(t, sh.ProjectID, PROJECT)
		require.Equal(t, sh.RealSize, 7*chunkSize)

		report, err := chunker.NewVerifyContext(st, 1, id, true).VerifyChecksum(ctx)
		require.NoError(t, err)
		require.Equal(t, report.Status, chunker.StatusOk)
	}) {
		t.Fatal(err)
	}
}

func TestMove(t *testing.T) {
	ctx, ctxCancel := createTestContext(t)
	defer ctxCancel()

	conf, err := config.GetConfig()
	require.NoError(t, err)

	if len(conf.Nbs) == 0 {
		t.Log("Skipping move test due to unconfigured NBS endpoint")
		t.SkipNow()
	}

	facade, err := NewFacade(ctx, &conf)
	require.NoError(t, err)
	defer facade.Close(ctx)

	// Create snapshot
	sh1 := snapshot{}
	sh1.ID = makeUUID(t)
	sh1.Size = 16 * chunkSize
	sh1.ProjectID = PROJECT
	sh1.Name = sh1.ID
	_, err = facade.CreateSnapshot(ctx, &sh1.CreationInfo)
	require.NoError(t, err)
	defer facade.DeleteSnapshot(ctx, &common.DeleteRequest{ID: sh1.ID})

	// Fill snapshot
	ch := facade.getChunker()
	data := make([]byte, sh1.Size)
	_, err = rand.Read(data[:sh1.Size/2])
	require.NoError(t, err)
	write(ctx, t, ch, &sh1, data, 0)

	// Commit snapshot
	err = facade.CommitSnapshot(ctx, sh1.ID, true)
	require.NoError(t, err)

	// Get NBS clusterID from config
	var clusterID string
	for clusterID = range conf.Nbs {
		break
	}
	require.NotEmpty(t, clusterID)

	// Create disk
	nbsClient, err := facade.mcf.GetMDF().GetNbsClient(ctx, clusterID)
	require.NoError(t, err)
	// defer nbsClient.Close()

	diskID := "test-disk-" + makeUUID(t)
	ctxlog.G(ctx).Debug("Creating volume", zap.String("diskID", diskID))
	err = nbsClient.CreateVolume(ctx, diskID, uint64(sh1.Size)/uint64(nbsBlockSize), &client.CreateVolumeOpts{
		BlockSize: nbsBlockSize,
	})
	require.NoError(t, err)
	defer func() {
		err := nbsClient.DestroyVolume(ctx, diskID)
		ctxlog.G(ctx).Debug("DestroyVolume", zap.Error(err))
	}()
	ctxlog.G(ctx).Debug("Created volume", zap.String("diskID", diskID))

	// Copy data from snapshot to disk
	taskID1 := "task-id-1"
	err = facade.Move(ctx, &common.MoveRequest{
		Src: &common.SnapshotMoveSrc{
			SnapshotID: sh1.ID,
		},
		Dst: &common.NbsMoveDst{
			ClusterID: clusterID,
			DiskID:    diskID,
		},
		Params: common.MoveParams{
			Offset:     0,
			SkipZeroes: false,
			BlockSize:  0,
			TaskID:     taskID1,
		},
	})
	require.NoError(t, err)
	defer facade.DeleteTask(ctx, taskID1)

	// Wait for task to finish
	var status *common.TaskStatus
	for i := 0; i < 60; i++ {
		status, err = facade.GetTaskStatus(ctx, taskID1)
		require.NoError(t, err)
		ctxlog.G(ctx).Debug("Moving", zap.Any("status", *status))

		if status.Finished {
			break
		} else {
			time.Sleep(time.Second)
		}
	}
	require.True(t, status.Finished)
	require.True(t, status.Success)
	require.NoError(t, status.Error)

	// Interrupt move to check cleanup
	taskID2 := "task-id-2"
	err = facade.Move(ctx, &common.MoveRequest{
		Src: &common.SnapshotMoveSrc{
			SnapshotID: sh1.ID,
		},
		Dst: &common.NbsMoveDst{
			ClusterID: clusterID,
			DiskID:    diskID,
		},
		Params: common.MoveParams{
			Offset:             0,
			SkipZeroes:         false,
			BlockSize:          0,
			TaskID:             taskID2,
			HeartbeatEnabled:   true,
			HeartbeatTimeoutMs: 10,
		},
	})
	require.NoError(t, err)
	defer facade.DeleteTask(ctx, taskID2)

	time.Sleep(time.Second * 5)
	status, err = facade.GetTaskStatus(ctx, taskID2)
	require.EqualError(t, err, misc.ErrTaskNotFound.Error())

	// Create second snapshot
	sh2 := sh1
	sh2.ID = makeUUID(t)
	sh2.Name = sh2.ID
	_, err = facade.CreateSnapshot(ctx, &sh2.CreationInfo)
	require.NoError(t, err)
	defer facade.DeleteSnapshot(ctx, &common.DeleteRequest{ID: sh2.ID})

	err = nbsClient.CreateCheckpoint(ctx, diskID, "checkpoint")
	require.NoError(t, err)
	defer nbsClient.DeleteCheckpoint(ctx, diskID, "checkpoint")

	// Copy data from disk to snapshot
	taskID3 := "task-id-3"
	err = facade.Move(ctx, &common.MoveRequest{
		Src: &common.NbsMoveSrc{
			ClusterID:        clusterID,
			DiskID:           diskID,
			LastCheckpointID: "checkpoint",
		},
		Dst: &common.SnapshotMoveDst{
			SnapshotID: sh2.ID,
		},
		Params: common.MoveParams{
			Offset:     0,
			SkipZeroes: true,
			BlockSize:  0,
			TaskID:     taskID3,
		},
	})
	require.NoError(t, err)
	defer facade.DeleteTask(ctx, taskID3)

	// Wait for task to finish
	for i := 0; i < 60; i++ {
		status, err = facade.GetTaskStatus(ctx, taskID3)
		require.NoError(t, err)
		ctxlog.G(ctx).Debug("Moving", zap.Any("status", *status))

		if status.Finished {
			break
		} else {
			time.Sleep(time.Second)
		}
	}
	require.True(t, status.Finished)
	require.True(t, status.Success)
	require.NoError(t, status.Error)

	// Commit snapshot
	err = facade.CommitSnapshot(ctx, sh2.ID, true)
	require.NoError(t, err)

	// Compare result
	read(ctx, t, ch, &sh2, data)

	chunks, err := facade.st.GetChunksFromCache(ctx, sh2.ID, false)
	require.NoError(t, err)

	require.Equal(t, sh1.Size/chunkSize/2, int64(chunks.Len()))
}

func TestDeleteContext(t *testing.T) {
	ctx, ctxCancel := createTestContext(t)
	defer ctxCancel()

	conf, err := config.GetConfig()
	require.NoError(t, err)

	f, err := NewFacadeOps(ctx, &conf, misc.TableOpNone)
	require.NoError(t, err)
	defer f.Close(ctx)

	data := make([]byte, 8*chunkSize)
	rand.Read(data)

	var sh snapshot
	sh.Name = NAME
	sh.Size = int64(len(data))
	sh.ID = makeUUID(t)

	ch := f.getChunker()
	cycle(ctx, t, ch, f.st, &sh, data, len(data))

	taskID := makeUUID(t)
	err = f.DeleteSnapshot(ctx, &common.DeleteRequest{
		ID: sh.ID,
		Params: common.DeleteParams{
			TaskID: taskID,
		},
	})
	require.NoError(t, err)

	var status *common.TaskStatus
	for i := 0; i < 200; i++ {
		status, err = f.GetTaskStatus(ctx, taskID)
		require.NoError(t, err)

		if status.Finished {
			require.NoError(t, err)
			break
		}
		time.Sleep(100 * time.Millisecond)
	}

	if !status.Finished {
		t.Fatal("Delete wait timeout")
	}

	cleared, err := f.st.IsSnapshotCleared(ctx, sh.ID)
	require.NoError(t, err)
	require.Equal(t, true, cleared)
}

func TestVerifySnapshot(t *testing.T) {
	ctx, ctxCancel := createTestContext(t)
	defer ctxCancel()

	conf, err := config.GetConfig()
	require.NoError(t, err)

	facade, err := NewFacade(ctx, &conf)
	require.NoError(t, err)
	defer func() {
		require.NoError(t, facade.Close(ctx))
	}()

	// Create snapshot
	sh := snapshot{}
	sh.ID = makeUUID(t)
	sh.Size = 16 * chunkSize
	sh.ProjectID = PROJECT
	sh.Name = sh.ID
	_, err = facade.CreateSnapshot(ctx, &sh.CreationInfo)
	require.NoError(t, err)
	defer func() {
		require.NoError(t, facade.DeleteSnapshot(ctx, &common.DeleteRequest{ID: sh.ID}))
	}()

	// Fill snapshot
	ch := facade.getChunker()
	data := make([]byte, sh.Size)
	_, err = rand.Read(data[:sh.Size/2])
	require.NoError(t, err)
	write(ctx, t, ch, &sh, data, 0)
	require.NoError(t, facade.CommitSnapshot(ctx, sh.ID, true))

	// Check if snapshot filled correctly
	read(ctx, t, ch, &sh, data)

	// Break one chunk
	chunk, body, err := facade.st.ReadChunkBody(ctx, sh.ID, 0)
	require.NoError(t, err)

	// break some symbol in the middle of chunk
	prevSymbol := body[len(body)/2]
	body[len(body)/2] = prevSymbol + 1 // in go 255 + 1 is 0

	continueSnapshot := func() {
		p, err := kikimrprovider.NewDatabaseProvider(conf.Kikimr)
		require.NoError(t, err)
		db, err := p.GetDatabase(defaultDatabase).Open(ctx, nil)
		require.NoError(t, err)
		_, err = db.ExecContext(ctx, "DECLARE $state AS Utf8; DECLARE $id AS Utf8;"+
			"UPDATE "+fmt.Sprintf("[%s/snapshotsext]", conf.Kikimr[defaultDatabase].Root)+" SET state=$state WHERE id=$id",
			sql.Named("state", storage.StateCreating), sql.Named("id", sh.ID))
		require.NoError(t, err)
	}

	continueSnapshot()
	require.NoError(t, facade.st.BeginChunk(ctx, sh.ID, 0))
	require.NoError(t, facade.st.EndChunk(ctx, sh.ID, chunk, 0, body, 0))
	require.NoError(t, facade.CommitSnapshot(ctx, sh.ID, true))

	checkCorrupted := func() {
		taskID := makeUUID(t)

		err = facade.Move(ctx, &common.MoveRequest{
			Src: &common.SnapshotMoveSrc{SnapshotID: sh.ID},
			Dst: &common.NullMoveDst{
				Size:      sh.Size,
				BlockSize: chunkSize,
			},
			Params: common.MoveParams{TaskID: taskID},
		})
		require.NoError(t, err)
		defer func() {
			require.NoError(t, facade.DeleteTask(ctx, taskID))
		}()

		var status *common.TaskStatus
		for i := 0; i < 200; i++ {
			status, err = facade.GetTaskStatus(ctx, taskID)
			require.NoError(t, err)

			if status.Finished {
				require.NoError(t, err)
				break
			}
			time.Sleep(100 * time.Millisecond)
		}

		if !status.Finished {
			t.Fatal("Move wait timeout")
		}

		require.True(t, xerrors.Is(status.Error, misc.ErrCorruptedSource), status.Error)
	}

	// check chunk checksum fail
	checkCorrupted()

	// Fix one chunk
	body[len(body)/2] = prevSymbol
	continueSnapshot()
	require.NoError(t, facade.st.BeginChunk(ctx, sh.ID, 0))
	require.NoError(t, facade.st.EndChunk(ctx, sh.ID, chunk, 0, body, 0))
	require.NoError(t, facade.CommitSnapshot(ctx, sh.ID, true))

	// Check if snapshot readed correctly again
	read(ctx, t, ch, &sh, data)

	// break snapshot checksum
	info, err := facade.st.GetSnapshotFull(ctx, sh.ID)
	require.NoError(t, err)
	require.NoError(t, facade.st.UpdateSnapshotChecksum(ctx, sh.ID, info.Checksum+"broken-global-checksum"))

	// check global checksum fail
	checkCorrupted()
}

func TestNoDeadlockInOperations(t *testing.T) {
	ctx, ctxCancel := createTestContext(t)
	defer ctxCancel()

	conf, err := config.GetConfig()
	require.NoError(t, err)

	facade, err := NewFacade(ctx, &conf)
	require.NoError(t, err)
	defer func() {
		require.NoError(t, facade.Close(ctx))
	}()

	// Create snapshot
	sh := snapshot{}
	sh.ID = makeUUID(t)
	sh.Size = 64 * chunkSize
	sh.ProjectID = PROJECT
	sh.Name = sh.ID
	_, err = facade.CreateSnapshot(ctx, &sh.CreationInfo)
	require.NoError(t, err)

	// Fill snapshot
	ch := facade.getChunker()
	data := make([]byte, sh.Size)
	_, err = rand.Read(data[:sh.Size/2])
	require.NoError(t, err)
	write(ctx, t, ch, &sh, data, 0)
	require.NoError(t, facade.CommitSnapshot(ctx, sh.ID, true))

	// Check if snapshot filled correctly
	read(ctx, t, ch, &sh, data)

	taskID := makeUUID(t)
	secondID := makeUUID(t)

	require.NoError(t, facade.Move(ctx, &common.MoveRequest{
		Src: &common.SnapshotMoveSrc{SnapshotID: sh.ID},
		Dst: &common.SnapshotMoveDst{
			SnapshotID: secondID,
			Create:     true,
			Commit:     true,
			Fail:       true,
			Name:       secondID,
		},
		Params: common.MoveParams{TaskID: taskID},
	}))

	status, err := facade.GetTaskStatus(ctx, taskID)
	require.NoError(t, err)
	require.NoError(t, status.Error)

	checkLocked := func() {
		p, err := kikimrprovider.NewDatabaseProvider(conf.Kikimr)
		require.NoError(t, err)
		db, err := p.GetDatabase(defaultDatabase).Open(ctx, nil)
		require.NoError(t, err)

		checkDeadline := time.Now().Add(time.Second * 10)
		needCheck := map[string]struct{}{
			sh.ID:    {},
			secondID: {},
		}
		for time.Now().Before(checkDeadline) {
			r, err := db.QueryContext(ctx, "DECLARE $id1 AS Utf8; DECLARE $id2 AS Utf8; "+
				" SELECT id, lock_state FROM "+fmt.Sprintf("[%s/snapshots_locks]", conf.Kikimr[defaultDatabase].Root)+" WHERE id=$id1 or id=$id2",
				sql.Named("id1", sh.ID),
				sql.Named("id2", secondID))
			if kikimr.IsKikimrRetryableErr(err) {
				time.Sleep(100 * time.Millisecond)
				continue
			}
			require.NoError(t, err)
			require.NoError(t, r.Err())
			var lock serializedlock.Lock
			var lockJSON string
			var id string
			for r.Next() {
				require.NoError(t, r.Scan(&id, &lockJSON))
				require.NoError(t, json.Unmarshal([]byte(lockJSON), &lock))
				if _, ok := needCheck[id]; lock.IsLocked(time.Now()) && ok {
					delete(needCheck, id)
				}
			}
			if len(needCheck) == 0 {
				return
			}
			time.Sleep(100 * time.Millisecond)
		}
		t.Fatal("snapshots are not locked")
	}

	checkLocked()

	// wait
	convertDeadline := time.Now().Add(time.Minute * 5)
	for time.Now().Before(convertDeadline) {
		status, err := facade.GetTaskStatus(ctx, taskID)
		require.NoError(t, err)
		require.NoError(t, status.Error)
		if status.Finished {
			break
		}
		time.Sleep(100 * time.Millisecond)
	}

	// no deadlock inside
	status, err = facade.GetTaskStatus(ctx, taskID)
	require.NoError(t, err)
	require.NoError(t, status.Error)
	require.True(t, status.Finished)

	// now snapshots are unlocked
	_, holder, err := facade.st.LockSnapshot(ctx, sh.ID, "")
	require.NoError(t, err)
	holder.Close(ctx)

	_, holder, err = facade.st.LockSnapshot(ctx, secondID, "")
	require.NoError(t, err)
	holder.Close(ctx)
}

func TestNoCancelTaskRace(t *testing.T) {
	ctx, ctxCancel := createTestContext(t)
	defer ctxCancel()

	size := int64(1024 * misc.GB)

	conf, err := config.GetConfig()
	require.NoError(t, err)

	facade, err := NewFacade(ctx, &conf)
	require.NoError(t, err)
	defer func() {
		require.NoError(t, facade.Close(ctx))
	}()
	taskID := makeUUID(t)
	deviceCloseError := make(chan error)
	cbk := func(_ context.Context, e error) {
		deviceCloseError <- e
		close(deviceCloseError)
	}
	require.NoError(t, facade.Move(ctx, &common.MoveRequest{Src: &common.NullMoveSrc{Size: size, BlockSize: 1}, Dst: &common.NullMoveDst{Size: size, BlockSize: 1, CloseCallback: cbk}, Params: common.MoveParams{TaskID: taskID, WorkersCount: 1}}))
	var st *common.TaskStatus
	for i := 0; i < 200; i++ {
		st, err = facade.GetTaskStatus(ctx, taskID)
		require.NoError(t, err)
		require.False(t, st.Finished, "too fast execution")
		if st.Offset > 0 {
			break
		}
		time.Sleep(100 * time.Millisecond)
	}
	require.NotEqual(t, st.Offset, 0)

	require.NoError(t, facade.CancelTask(ctx, taskID))

	for i := 0; i < 200; i++ {
		st, err = facade.GetTaskStatus(ctx, taskID)
		require.NoError(t, err)

		if st.Finished {
			break
		}
		time.Sleep(100 * time.Millisecond)
	}

	if !st.Finished {
		t.Fatal("Failed to cancel task")
	}

	require.True(t, xerrors.Is(st.Error, misc.ErrTaskCancelled), st.Error)
	err = <-deviceCloseError
	require.True(t, xerrors.Is(err, misc.ErrTaskCancelled), "this error should be ErrTaskCancelled so device know task is cancelled and job is not done")
}
