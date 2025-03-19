package kikimr

import (
	"context"
	"fmt"
	"io"
	"path"
	"sort"
	"testing"
	"time"

	"github.com/gofrs/uuid"
	"github.com/opentracing/opentracing-go"
	"github.com/stretchr/testify/assert"
	"go.uber.org/zap"
	"go.uber.org/zap/zaptest"

	"a.yandex-team.ru/cloud/compute/go-common/logging/ctxlog"
	"a.yandex-team.ru/cloud/compute/go-common/tracing"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/common"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/config"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/misc"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/storage"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/table"
)

type testEnv struct {
	a      *assert.Assertions
	st     storage.Storage
	ctx    context.Context
	cancel context.CancelFunc
	conf   config.Config

	tracer io.Closer
	span   opentracing.Span
}

func newTestEnv(t *testing.T) testEnv {
	ctx := ctxlog.WithLogger(context.Background(), zaptest.NewLogger(t).WithOptions(zap.Development()))
	ctxlog.G(ctx).Debug("creating new test enviroment", zap.String("test_name", t.Name()))
	ctx, cancel := context.WithCancel(ctx)
	cfg, err := config.GetConfig()
	if err != nil {
		ctxlog.G(ctx).Panic("error on getting config", zap.Error(err))
	}
	storageI, err := NewKikimr(ctx, &cfg, misc.TableOpCreate)
	if err != nil {
		ctxlog.G(ctx).Panic("error on creating test kikimr", zap.Error(err))
	}
	if err = storageI.PingContext(ctx); err != nil {
		ctxlog.G(ctx).Panic("error on connection to db check", zap.Error(err))
	}

	tracer, err := tracing.InitJaegerTracing(cfg.Tracing.Config)
	if err != nil {
		panic(err)
	}

	var span opentracing.Span
	span, ctx = tracing.InitSpan(ctx, t.Name())

	return testEnv{
		st:     storageI,
		ctx:    ctx,
		a:      assert.New(t),
		cancel: cancel,
		conf:   cfg,

		tracer: tracer,
		span:   span,
	}
}

func (t *testEnv) Close() {
	t.cancel()
	_ = t.tracer.Close()
	t.span.Finish()
}

func init() {
	InitConfig()
}

func TestCreateReadSnapshot(t *testing.T) {
	te := newTestEnv(t)
	defer te.Close()

	// Bare minimum config for creating snapshot
	id, err := te.st.BeginSnapshot(te.ctx, &common.CreationInfo{
		CreationMetadata: common.CreationMetadata{
			ID:   uuid.Must(uuid.NewV4()).String(),
			Name: "testCreateSnapshot",
		},
		Size: 4 * storage.DefaultChunkSize,
	})
	te.a.NoError(err)

	data := [4][]byte{}
	data[0] = make([]byte, storage.DefaultChunkSize)
	data[1] = make([]byte, storage.DefaultChunkSize)
	data[2] = make([]byte, storage.DefaultChunkSize)
	data[3] = make([]byte, storage.DefaultChunkSize)
	for i := 0; i < storage.DefaultChunkSize; i++ {
		data[0][i] = byte(i)
		data[3][i] = byte(-i)
	}
	chunks := [4]*storage.LibChunk{
		{
			Format: storage.Raw,
			Size:   storage.DefaultChunkSize,
			Offset: 0,
		},
		{
			Format: storage.Raw,
			Size:   storage.DefaultChunkSize,
			Zero:   true,
			Offset: storage.DefaultChunkSize,
		},
		nil,
		{
			Format: storage.Raw,
			Size:   storage.DefaultChunkSize,
			Offset: 3 * storage.DefaultChunkSize,
		},
	}

	err = te.st.EndChunk(te.ctx, id, chunks[0], 0, data[0], 0)
	te.a.NoError(err)
	err = te.st.EndChunk(te.ctx, id, chunks[1], storage.DefaultChunkSize, nil, 0)
	te.a.NoError(err)
	// data[2] represents missing chunk so it is not added
	err = te.st.EndChunk(te.ctx, id, chunks[3], storage.DefaultChunkSize*3, data[3], 0)
	te.a.NoError(err)

	err = te.st.EndSnapshot(te.ctx, id)
	te.a.NoError(err)
	for i := int64(0); i < 4; i++ {
		readedChunk, dt, err := te.st.ReadChunkBody(te.ctx, id, i*storage.DefaultChunkSize)
		te.a.NoError(err)
		te.a.Equal(chunks[i], readedChunk)
		te.a.Equal(data[i], dt, "read wrong data from snapshot")
	}
}

func TestChunk(t *testing.T) {
	te := newTestEnv(t)
	defer te.Close()

	id, err := te.st.BeginSnapshot(te.ctx, &common.CreationInfo{
		CreationMetadata: common.CreationMetadata{
			ID:   uuid.Must(uuid.NewV4()).String(),
			Name: "testblob",
		},
		Size: 24,
	})
	te.a.NoError(err)

	testdata := []byte("testtesttest")
	testblob := []byte("blobblobblob")
	chunk1 := storage.LibChunk{Size: int64(len(testdata))}

	err = te.st.BeginChunk(te.ctx, id, 0)
	te.a.NoError(err)
	err = te.st.EndChunk(te.ctx, id, &chunk1, 0, testdata, 0)
	te.a.NoError(err)
	chunk2 := storage.LibChunk{Size: int64(len(testblob))}

	err = te.st.BeginChunk(te.ctx, id, int64(len(testdata)))
	te.a.NoError(err)
	err = te.st.EndChunk(te.ctx, id, &chunk2, int64(len(testdata)), testblob, 0)
	te.a.NoError(err)
	err = te.st.EndSnapshot(te.ctx, id)
	te.a.NoError(err)

	data, err := te.st.ReadBlob(te.ctx, &chunk1, true)
	te.a.NoError(err)
	te.a.Equal(testdata, data)
	data, err = te.st.ReadBlob(te.ctx, &chunk2, true)
	te.a.NoError(err)
	te.a.Equal(testblob, data)

	_, err = te.st.ReadBlob(te.ctx, &storage.LibChunk{ID: "not exists"}, false)
	te.a.NoError(err)
	_, err = te.st.ReadBlob(te.ctx, &storage.LibChunk{ID: "not exists"}, true)
	te.a.Equal(misc.ErrNoSuchChunk, err)
}

func TestGetTreeSnapshot(t *testing.T) {
	te := newTestEnv(t)
	defer te.Close()

	base, err := te.st.BeginSnapshot(te.ctx, &common.CreationInfo{
		CreationMetadata: common.CreationMetadata{
			ProjectID: "bpid",
			ID:        uuid.Must(uuid.NewV4()).String(),
			Name:      "base",
		},
		Size: 24,
	})
	te.a.NoError(err)
	baseChunk := storage.LibChunk{Size: 12, Format: storage.Raw, Offset: 12}
	err = te.st.EndChunk(te.ctx, base, &baseChunk, 12, nil, 0)
	te.a.NoError(err)
	err = te.st.EndSnapshot(te.ctx, base)
	te.a.NoError(err)
	tCreating := time.Now()
	id, err := te.st.BeginSnapshot(te.ctx, &common.CreationInfo{
		CreationMetadata: common.CreationMetadata{
			UpdatableMetadata: common.UpdatableMetadata{
				Metadata:    "meta",
				Description: "test snapshot",
			},
			ID:           uuid.Must(uuid.NewV4()).String(),
			Organization: "idk",
			ProjectID:    "prid",
			ProductID:    "pid",
			ImageID:      "iid",
			Name:         "testGetSnapshot",
			Public:       true,
		},
		Base: base,
		Size: 24,
		Disk: "disk",
	})
	te.a.NoError(err)
	chunk := storage.LibChunk{
		Format: storage.Raw,
		Size:   12,
		Zero:   true,
		Offset: 0,
	}
	err = te.st.EndChunk(te.ctx, id, &chunk, 0, nil, 0)
	te.a.NoError(err)
	err = te.st.EndSnapshot(te.ctx, id)
	te.a.NoError(err)

	info, err := te.st.GetSnapshot(te.ctx, id)
	te.a.NoError(err)
	te.a.True(info.Created.After(tCreating))
	info.Created = time.Time{}
	te.a.True(info.Changes[0].Timestamp.After(tCreating))
	info.Changes[0].Timestamp = time.Time{}
	te.a.Equal(&common.SnapshotInfo{
		CreationInfo: common.CreationInfo{
			CreationMetadata: common.CreationMetadata{
				UpdatableMetadata: common.UpdatableMetadata{
					Metadata:    "meta",
					Description: "test snapshot",
				},
				ID:           id,
				Organization: "idk",
				ProjectID:    "prid",
				ProductID:    "pid",
				ImageID:      "iid",
				Name:         "testGetSnapshot",
				Public:       true,
			},
			Base: base,
			Size: 24,
			Disk: "disk",
		},
		Changes: []common.SizeChange{{
			RealSize: 0,
		}},
		RealSize: 0,
		State:    common.State{Code: "ready"},
	}, info)

	data, err := te.st.GetSnapshotInternal(te.ctx, id)
	te.a.NoError(err)
	te.a.Equal(&storage.SnapshotInternal{
		ChunkSize: 12,
		Tree:      base,
	}, data)

	chunkMap, err := te.st.GetChunksFromCache(te.ctx, id, false)
	te.a.NoError(err)
	res := storage.NewChunkMap()
	res.Insert(0, &chunk)
	res.Insert(12, &baseChunk)
	te.a.Equal(res, chunkMap)
}

func TestSnapshotLifecycle(t *testing.T) {
	te := newTestEnv(t)
	defer te.Close()

	// Init snapshot
	id, err := te.st.BeginSnapshot(te.ctx, &common.CreationInfo{
		CreationMetadata: common.CreationMetadata{
			UpdatableMetadata: common.UpdatableMetadata{
				Metadata:    "meta",
				Description: "test snapshot",
			},
			ID:   uuid.Must(uuid.NewV4()).String(),
			Name: "testSnapshotLifecycle",
		},
		Size: 12 * misc.MB,
	})
	te.a.NoError(err)

	// Fill snapshot
	chunk := storage.LibChunk{
		Format: storage.Raw,
		Size:   12,
		Zero:   true,
	}
	err = te.st.EndChunk(te.ctx, id, &chunk, 0, nil, 0)
	te.a.NoError(err)

	// Update snapshot before commit
	checksum := "^_^"
	err = te.st.UpdateSnapshotChecksum(te.ctx, id, checksum)
	te.a.NoError(err)
	newCodeDescr := "someone silly changed this description:("
	err = te.st.UpdateSnapshotStatus(te.ctx, id, storage.StatusUpdate{
		State: storage.StateCreating,
		Desc:  newCodeDescr,
	})
	te.a.NoError(err)
	internals, err := te.st.GetSnapshotFull(te.ctx, id)
	te.a.NoError(err)
	te.a.Equal(checksum, internals.Checksum)
	te.a.Equal("", internals.State.Description) // expected behaviour because of database read optimization
	info, err := te.st.GetSnapshot(te.ctx, id)
	te.a.NoError(err)
	te.a.Equal(newCodeDescr, info.State.Description)

	// Commit (freeze) snapshot
	err = te.st.EndSnapshot(te.ctx, id)
	te.a.NoError(err)

	// Update some updatable snapshot fields
	newMeta := "newMeta"
	newDescr := "brand new description"
	err = te.st.UpdateSnapshot(te.ctx, &common.UpdateRequest{
		ID:          id,
		Metadata:    &newMeta,
		Description: &newDescr,
	})
	te.a.NoError(err)
	info, err = te.st.GetSnapshot(te.ctx, id)
	te.a.NoError(err)
	te.a.Equal(newMeta, info.Metadata)
	te.a.Equal(newDescr, info.Description)

	// Go to deleting state
	err = te.st.BeginDeleteSnapshot(te.ctx, id)
	te.a.NoError(err)

	// Checks if snapshot is not in deleting state (it is)
	_, err = te.st.GetLiveSnapshot(te.ctx, id)
	te.a.Equal(misc.ErrSnapshotNotFound, err)

	// Delete chunks and row from snapshot table
	err = te.st.EndDeleteSnapshotSync(te.ctx, id, false)
	te.a.NoError(err)
	chunkMap, err := te.st.GetChunksFromCache(te.ctx, id, true)
	te.a.NoError(err)
	te.a.Equal(storage.NewChunkMap(), chunkMap)
	te.a.True(te.st.IsSnapshotCleared(te.ctx, id))

	// Delete remained meta data, now nothing can be found
	err = te.st.DeleteTombstone(te.ctx, id)
	te.a.NoError(err)
	_, err = te.st.GetSnapshot(te.ctx, id)
	te.a.Equal(misc.ErrSnapshotNotFound, err)
}

func TestListGC(t *testing.T) {
	te := newTestEnv(t)
	defer te.Close()
	ids := make([]string, 3)
	for i := 0; i < 3; i++ {
		id, err := te.st.BeginSnapshot(te.ctx, &common.CreationInfo{
			CreationMetadata: common.CreationMetadata{
				ID:   uuid.Must(uuid.NewV4()).String(),
				Name: fmt.Sprintf("testListAndCopy%d", i),
			},
			Size: 4 * storage.DefaultChunkSize,
		})
		te.a.NoError(err)
		data := make([]byte, storage.DefaultChunkSize)
		for j := int64(0); j < storage.DefaultChunkSize; j++ {
			data[j] = byte(i + 1)
		}
		err = te.st.EndChunk(te.ctx, id, &storage.LibChunk{
			Format: storage.Raw,
			Size:   storage.DefaultChunkSize,
		}, 0, data, 0)
		te.a.NoError(err)
		ids[i] = id
	}
	info, err := te.st.ListGC(te.ctx, &storage.GCListRequest{
		N:              2,
		FailedCreation: time.Nanosecond,
	})
	te.a.NoError(err)
	acquiredIDs := []string{info[0].ID, info[1].ID}
	sort.Strings(ids)

	te.a.Equal(ids[:2], acquiredIDs)

	for _, id := range ids {
		err := te.st.EndSnapshot(te.ctx, id)
		te.a.NoError(err)
	}
}

func TestCopyChunkRefs(t *testing.T) {
	te := newTestEnv(t)
	defer te.Close()

	id, err := te.st.BeginSnapshot(te.ctx, &common.CreationInfo{
		CreationMetadata: common.CreationMetadata{
			ID:   uuid.Must(uuid.NewV4()).String(),
			Name: "testChunksRef",
		},
		Size: 4 * storage.DefaultChunkSize,
	})
	te.a.NoError(err)
	data := make([]byte, storage.DefaultChunkSize)
	for i := int64(0); i < storage.DefaultChunkSize; i++ {
		data[i] = byte(i ^ (i + 1) + 5)
	}
	chunk := storage.LibChunk{
		Format: storage.Raw,
		Size:   storage.DefaultChunkSize,
	}
	err = te.st.EndChunk(te.ctx, id, &chunk, storage.DefaultChunkSize, data, 0)
	te.a.NoError(err)
	err = te.st.EndSnapshot(te.ctx, id)
	te.a.NoError(err)

	err = te.st.CopyChunkRefs(te.ctx, id, id, []storage.ChunkRef{{Offset: 0, ChunkID: chunk.ID}})
	te.a.NoError(err)

	_, dt, err := te.st.ReadChunkBody(te.ctx, id, 0)
	te.a.NoError(err)
	te.a.Equal(data, dt)

	_, dt, err = te.st.ReadChunkBody(te.ctx, id, storage.DefaultChunkSize)
	te.a.NoError(err)
	te.a.Equal(data, dt)
}

func TestCreateAndDeleteTables(t *testing.T) {
	te := newTestEnv(t)
	defer te.Close()

	kikimr, ok := te.st.(*kikimrstorage)
	te.a.True(ok, "non-kikimr storage found")

	blobsPartitions := 0
	newTables := []tableDescription{
		{
			name: "testtable",
			columns: []columnDescription{
				{"id", "string", false},
				{"base", "string", false},
				{"created", "uint64", false},
				{"size", "int64", false},
				{"public", "bool", false},
			},
			keyColumns:    []string{"base", "id"},
			autoSplitSize: misc.MB,
			startSplit:    nil,
		},
		{
			"testtable_external",
			[]columnDescription{
				{"id", "string", false},
				{"data", "string", true},
				{"state", "json", false},
			}, []string{"id"}, 0,
			uuidSplitter{&blobsPartitions},
		},
	}

	// create and register tables
	err := kikimr.Init(te.ctx, misc.TableOpCreate, newTables)
	te.a.NoError(err)

	queryRegexp = compileQueryRegexp(append(buildTables(0), newTables...))
	defer func() {
		queryRegexp = compileQueryRegexp(buildTables(0))
	}()

	chunk := make([]byte, storage.DefaultChunkSize)
	for i := 0; i < storage.DefaultChunkSize; i++ {
		chunk[i] = 'a'
	}

	uint64Val := uint64(1)
	int64Val := int64(2)
	boolVal := true
	primKeyVal := "id"
	secondKeyVal := "base"
	jsonVal := `{"a":"b"}`

	err = kikimr.retryWithTx(te.ctx, "test.Populate", func(tx Querier) error {
		_, err := tx.ExecContext(te.ctx,
			"INSERT INTO $testtable (id, base, created, size, public)"+
				" VALUES ($1, $2, $3, $4, $5)", primKeyVal, secondKeyVal, uint64Val, int64Val, boolVal)
		te.a.NoError(err)

		_, err = tx.ExecContext(te.ctx,
			"INSERT INTO  $testtable_external (id, data, state) VALUES ($1, $2, $3)",
			primKeyVal, string(chunk), jsonVal)
		te.a.NoError(err)
		return err
	})
	te.a.NoError(err)

	err = kikimr.retryWithTx(te.ctx, "test.Get", func(tx Querier) error {
		res, err := tx.QueryContext(te.ctx, "SELECT id, base, created, size, public FROM $testtable")
		te.a.NoError(err)

		te.a.True(res.Next())
		var (
			id, base string
			created  uint64
			size     int64
			public   bool
		)
		err = res.Scan(&id, &base, &created, &size, &public)
		te.a.NoError(err)
		te.a.Equal(primKeyVal, id)
		te.a.Equal(secondKeyVal, base)
		te.a.Equal(uint64Val, created)
		te.a.Equal(int64Val, size)
		te.a.Equal(boolVal, public)
		te.a.False(res.Next())
		err = res.Close()
		te.a.NoError(err)

		res, err = tx.QueryContext(te.ctx, "SELECT id, data, state FROM $testtable_external")
		te.a.NoError(err)
		te.a.True(res.Next())
		var (
			json string
			blob []byte
		)
		err = res.Scan(&id, &blob, &json)
		te.a.NoError(err)
		te.a.Equal(primKeyVal, id)
		te.a.Equal(chunk, blob)
		te.a.Equal(jsonVal, json)
		te.a.False(res.Next())
		err = res.Close()
		te.a.NoError(err)
		return err
	})
	te.a.NoError(err)

	// drop all the new temp tables
	err = kikimr.dropTables(te.ctx, newTables)
	te.a.NoError(err)
	_ = kikimr.retryWithTx(te.ctx, "testSelect", func(tx Querier) error {
		// try to select from dropped table
		_, err := tx.QueryContext(te.ctx, "SELECT id, data, state FROM $testtable_external")
		te.a.Error(err)
		te.a.True(ydb.IsOpError(err, ydb.StatusSchemeError) || ok)
		return nil
	})
}

func TestRootDirectory(t *testing.T) {
	te := newTestEnv(t)
	defer te.Close()
	kikimrConf := te.conf.Kikimr[defaultDatabase]
	oldConf := kikimrConf
	defer func() {
		te.conf.Kikimr[defaultDatabase] = oldConf
	}()

	kikimrConf = te.conf.Kikimr[defaultDatabase]
	kikimrConf.Root = path.Join("/"+kikimrConf.DBName, "tables", "items")
	te.conf.Kikimr[defaultDatabase] = kikimrConf
	_, err := NewKikimr(te.ctx, &te.conf, misc.TableOpCreate)
	te.a.NoError(err)
}

func TestHealthcheck(t *testing.T) {
	te := newTestEnv(t)
	defer te.Close()

	// important assertion - all queries in Health() worked as intended
	te.a.NoError(te.st.Health(context.Background()))

	te.a.NoError(te.st.Close())
	te.a.Error(te.st.Health(context.Background()))
}

func TestNewDriverTableProfile(t *testing.T) {
	te := newTestEnv(t)
	defer te.Close()

	kikimr, ok := te.st.(*kikimrstorage)
	te.a.True(ok, "non-kikimr storage found")

	driver, err := ydb.Dial(te.ctx, kikimr.p.GetDatabase(defaultDatabase).GetConfig().DBHost,
		&ydb.DriverConfig{Database: kikimr.p.GetDatabase(defaultDatabase).GetConfig().DBName})
	te.a.NoError(err)
	cl := table.Client{Driver: driver}
	session, err := cl.CreateSession(te.ctx)
	te.a.NoError(err)
	defer func() {
		_ = session.Close(te.ctx)
	}()

	descr, err := session.DescribeTable(te.ctx, fmt.Sprintf("%s/blobs_on_hdd", kikimr.p.GetDatabase(defaultDatabase).GetConfig().Root), table.WithTableStats())
	te.a.NoError(err)
	te.a.Greater(len(descr.ColumnFamilies), 0)
	var found bool
	for _, p := range descr.ColumnFamilies {
		if p.Name == bigTableProfileName && p.Data.Media == "hdd" {
			found = true
			break
		}
	}
	te.a.True(found, "we found required column family")
	// 1 profile is default one, another for blobs_on_hdd
	te.a.Greater(descr.Stats.Partitions, uint64(1))
}

func TestDeleteNotSavedChunks(t *testing.T) {
	te := newTestEnv(t)
	defer te.Close()

	id, err := te.st.BeginSnapshot(te.ctx, &common.CreationInfo{
		CreationMetadata: common.CreationMetadata{
			ID:   "snapshot",
			Name: "snapshot",
		},
		Size: storage.DefaultChunkSize,
	})
	te.a.NoError(err)

	data := make([]byte, storage.DefaultChunkSize)
	for i := range data {
		data[i] = byte(i)
	}
	chunk := storage.LibChunk{
		Format: storage.Raw,
		Size:   storage.DefaultChunkSize,
	}
	te.a.NoError(te.st.EndChunk(te.ctx, id, &chunk, 0, data, 0))
	te.a.NoError(te.st.EndSnapshot(te.ctx, id))

	// read chunk
	returned, err := te.st.ReadBlob(te.ctx, &chunk, true)
	te.a.NoError(err)
	te.a.Equal(data, returned)

	kikimr, ok := te.st.(*kikimrstorage)
	te.a.True(ok)

	te.a.NoError(kikimr.retryWithTx(te.ctx, "delete metadata", func(tx Querier) error {
		_, err := tx.ExecContext(te.ctx, "DELETE FROM $chunks where id=$1", chunk.ID)
		if err != nil {
			return err
		}
		_, err = tx.ExecContext(te.ctx, "DELETE FROM $snapshotchunks where chunkid=$1", chunk.ID)
		if err != nil {
			return err
		}
		_, err = tx.ExecContext(te.ctx, "UPDATE $snapshotsext SET state=$1", storage.StateCreating)
		return err
	}))

	te.a.NoError(te.st.BeginDeleteSnapshot(te.ctx, id))
	te.a.NoError(te.st.EndDeleteSnapshotSync(te.ctx, id, false))
	te.a.NoError(kikimr.retryWithTx(te.ctx, "checkChunkDeleted", func(tx Querier) error {
		d, err := tx.QueryContext(te.ctx, "SELECT id from $blobs_on_hdd where id=$1", chunk.ID)
		if err != nil {
			return err
		}
		// chunk was deleted even though it is not in chunks or snapshotchunks
		te.a.False(d.Next())
		return nil
	}))
}

func TestStoreChunkData(t *testing.T) {
	te := newTestEnv(t)
	defer te.Close()

	snapshotID := uuid.Must(uuid.NewV4()).String()
	data := make([]byte, storage.DefaultChunkSize)
	for i := range data {
		data[i] = byte(i)
	}
	te.a.NoError(te.st.StoreChunkData(te.ctx, snapshotID, storage.DefaultChunkSize, data))

	_, err := te.st.ReadBlob(te.ctx, &storage.LibChunk{
		ID: newChunkID(snapshotID, storage.DefaultChunkSize),
	}, true)
	te.a.NoError(err)
}

func TestStoreChunkMetadata(t *testing.T) {
	te := newTestEnv(t)
	defer te.Close()

	// Bare minimum config for creating snapshot
	id, err := te.st.BeginSnapshot(te.ctx, &common.CreationInfo{
		CreationMetadata: common.CreationMetadata{
			ID:   uuid.Must(uuid.NewV4()).String(),
			Name: "testStoreChunkMetadataSnapshot",
		},
		Size: 2,
	})
	te.a.NoError(err)

	set := map[storage.LibChunk]struct{}{
		{
			Sum:    "sum1",
			Format: "raw",
			Size:   1,
			Offset: 1,
			Zero:   false,
		}: {}, {
			Sum:    "sum2",
			Format: "raw",
			Size:   1,
			Offset: 2,
			Zero:   true,
		}: {},
	}
	task := make([]storage.LibChunk, 0)
	for k := range set {
		task = append(task, k)
	}

	// can read snapshot now
	te.a.NoError(te.st.StoreChunksMetadata(te.ctx, id, task))

	te.a.NoError(te.st.EndSnapshot(te.ctx, id))

	chunks, err := te.st.GetChunksFromCache(te.ctx, id, true)
	te.a.NoError(err)
	te.a.NoError(chunks.Foreach(func(i int64, chunk *storage.LibChunk) error {
		chunk.ID = ""
		_, ok := set[*chunk]
		te.a.True(ok, *chunk)
		if ok {
			delete(set, *chunk)
		}
		return nil
	}))
	te.a.Equal(0, len(set))
}
