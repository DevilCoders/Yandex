package kikimr

import (
	"io/ioutil"
	"os"
	"path"
	"strings"
	"testing"
	"time"

	"golang.org/x/xerrors"

	"a.yandex-team.ru/cloud/compute/snapshot/internal/serializedlock"

	"github.com/stretchr/testify/assert"

	"go.uber.org/zap/zaptest"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/logging"
	"a.yandex-team.ru/library/go/test/yatest"

	"golang.org/x/net/context"

	"github.com/gofrs/uuid"

	"a.yandex-team.ru/cloud/compute/go-common/logging/ctxlog"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/config"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/misc"
)

func init() {
	InitConfig()
}

func initCtx() context.Context {
	logger := logging.SetupCliLogging()
	ctx := ctxlog.WithLogger(context.Background(), logger)
	return ctx
}

func InitConfig() {
	defaultConfig := os.Getenv("TEST_CONFIG_PATH")
	if defaultConfig == "" {
		defaultConfig = "cloud/compute/snapshot/config.toml"
	}
	defaultConfig = yatest.SourcePath(defaultConfig)

	err := config.LoadConfig(defaultConfig)
	if err != nil {
		panic(err)
	}

	conf, err := config.GetConfig()
	if err != nil {
		panic(err)
	}

	endpointBytes, err := ioutil.ReadFile("ydb_endpoint.txt")
	if err != nil {
		panic(err)
	}
	ydbEndpoint := strings.TrimSpace(string(endpointBytes))

	rootBytes, err := ioutil.ReadFile("ydb_database.txt")
	if err != nil {
		panic(err)
	}
	ydbRoot := strings.TrimSpace(string(rootBytes))
	if !strings.HasPrefix(ydbRoot, "/") {
		ydbRoot = "/" + ydbRoot
	}

	defaultDB := conf.Kikimr[defaultDatabase]
	defaultDB.DBHost = ydbEndpoint
	defaultDB.Root = path.Join(ydbRoot, "tables")
	defaultDB.DBName = ydbRoot
	conf.Kikimr[defaultDatabase] = defaultDB
}

func TestSnapshotLock(t *testing.T) {
	r := require.New(t)

	ctx := ctxlog.WithLogger(context.Background(), zaptest.NewLogger(t))
	config, err := config.GetConfig()
	r.NoError(err)
	storageI, err := NewKikimr(ctx, &config, misc.TableOpCreate)
	r.NoError(err)
	storage := storageI.(*kikimrstorage)

	snapshotID1 := uuid.Must(uuid.NewV4()).String()
	snapshotID2 := uuid.Must(uuid.NewV4()).String()

	// first lock
	_, holder, err := storage.LockSnapshot(ctx, snapshotID1, "")
	r.NoError(err)

	// double lock
	_, _, err = storage.LockSnapshot(ctx, snapshotID1, "")
	r.Error(err)

	// lock with other id
	_, holder2, err := storage.LockSnapshot(ctx, snapshotID2, "")
	r.NoError(err)

	holder2.Close(ctx)

	// unlock first
	holder.Close(ctx)

	// lock after unlock
	_, _, err = storage.LockSnapshot(ctx, snapshotID1, "")
	r.NoError(err)
}

func TestSnapshotLockShared(t *testing.T) {
	r := require.New(t)
	a := assert.New(t)

	ctx := ctxlog.WithLogger(context.Background(), zaptest.NewLogger(t))
	config, err := config.GetConfig()
	r.NoError(err)
	storageI, err := NewKikimr(ctx, &config, misc.TableOpCreate)
	r.NoError(err)
	storage := storageI.(*kikimrstorage)

	snapshotID := uuid.Must(uuid.NewV4()).String()

	// multi shared, then exclusive
	_, holder1, err := storage.LockSnapshotShared(ctx, snapshotID, "")
	a.NoError(err)

	_, holder2, err := storage.LockSnapshotShared(ctx, snapshotID, "")
	a.NoError(err)

	_, _, err = storage.LockSnapshot(ctx, snapshotID, "")
	a.Equal(misc.ErrAlreadyLocked, err)

	holder1.Close(ctx)

	holder2.Close(ctx)

	// exclusive, then shared
	_, holder1, err = storage.LockSnapshot(ctx, snapshotID, "")
	a.NoError(err)

	_, _, err = storage.LockSnapshotShared(ctx, snapshotID, "")
	a.Equal(misc.ErrAlreadyLocked, err)

	holder1.Close(ctx)
}

func TestSnapshotLockSharedMax(t *testing.T) {
	r := require.New(t)
	a := assert.New(t)

	ctx := ctxlog.WithLogger(context.Background(), zaptest.NewLogger(t))
	config, err := config.GetConfig()
	r.NoError(err)
	storage, err := NewKikimr(ctx, &config, misc.TableOpCreate)
	r.NoError(err)

	snapshotID := uuid.Must(uuid.NewV4()).String()

	// multi shared, then exclusive
	_, holder1, err := storage.LockSnapshotSharedMax(ctx, snapshotID, "holer-1", 2)
	a.NoError(err)
	//defer holder1.Close(ctx) - unlock later in code

	_, holder2, err := storage.LockSnapshotSharedMax(ctx, snapshotID, "holer-2", 2)
	a.NoError(err)
	defer holder2.Close(ctx)

	_, _, err = storage.LockSnapshotSharedMax(ctx, snapshotID, "exceed-max-count", 2)
	a.True(xerrors.Is(err, misc.ErrMaxShareLockExceeded))

	holder1.Close(ctx)

	_, holder3, err := storage.LockSnapshotSharedMax(ctx, snapshotID, "holer-3", 2)
	a.NoError(err)
	holder3.Close(ctx)
}

func TestSnapshotLockTimeSkew(t *testing.T) {
	r := require.New(t)
	a := assert.New(t)

	ctx := ctxlog.WithLogger(context.Background(), zaptest.NewLogger(t))
	config, err := config.GetConfig()
	r.NoError(err)
	storage, err := NewKikimr(ctx, &config, misc.TableOpCreate)
	r.NoError(err)

	snapshotID1 := uuid.Must(uuid.NewV4()).String()
	kikimrStorage := storage.(*kikimrstorage)

	// set explicit timeskew
	kikimrStorage.timeSkewInterval = 10 * time.Second

	// if snapshot locked now it is locked
	now := time.Now()

	r.NoError(kikimrStorage.retryWithTx(ctx, "lock snapshot", func(tx Querier) error {
		lock := serializedlock.Lock{}
		_, err = lock.Lock(serializedlock.Exclusive, now, now.Add(time.Second*20))
		a.NoError(err)
		return lockStore(ctx, tx, snapshotID1, now, lock)
	}))

	_, _, err = storage.LockSnapshot(ctx, snapshotID1, "")
	a.Equal(misc.ErrAlreadyLocked, err)

	// if it is locked 10+ seconds ago then now it is considered unlocked (even if 20 seconds not to passed)
	now = now.Add(-10 * time.Second)

	r.NoError(kikimrStorage.retryWithTx(ctx, "lock snapshot but with timeskew", func(tx Querier) error {
		lock := serializedlock.Lock{TimeSkew: 10 * time.Second.Nanoseconds()}
		_, err = lock.Lock(serializedlock.Exclusive, now, now.Add(time.Second*20))
		a.NoError(err)
		return lockStore(ctx, tx, snapshotID1, now, lock)
	}))

	_, _, err = storage.LockSnapshot(ctx, snapshotID1, "")
	a.NoError(err)
	a.Less(time.Since(now).Nanoseconds(), time.Second.Nanoseconds()*20)
}
