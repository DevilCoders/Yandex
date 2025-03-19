//go:build e2e

package e2e

import (
	"context"
	"fmt"
	"os"
	"path"
	"testing"

	"github.com/stretchr/testify/assert"
	ydbZap "github.com/ydb-platform/ydb-go-sdk-zap"
	"github.com/ydb-platform/ydb-go-sdk/v3"
	"github.com/ydb-platform/ydb-go-sdk/v3/sugar"
	"github.com/ydb-platform/ydb-go-sdk/v3/table"
	"github.com/ydb-platform/ydb-go-sdk/v3/trace"
	"go.uber.org/zap"
	"go.uber.org/zap/zaptest"

	"a.yandex-team.ru/cloud/bootstrap/ydb-lock"
)

const (
	prefix = "test"
	// Scheme is taken from https://bb.yandex-team.ru/projects/cloud/repos/ci/browse/ci/yc_ci/common/clients/kikimr/__init__.py?at=451fdf7b3c19d31bf3df373c6046fba587a9a5c1#37
	schema = `
PRAGMA TablePathPrefix("%s");
CREATE TABLE host_bootstrap_lock (
    hostname Utf8,
    deadline Uint64,
    description Utf8,
	type Utf8,
    PRIMARY KEY (hostname)
);
`
)

var (
	root = path.Join(os.Getenv("YDB_DATABASE"), prefix)
)

func conn(ctx context.Context, logger *zap.Logger) (ydb.Connection, error) {
	return ydb.Open(
		ctx,
		sugar.DSN(os.Getenv("YDB_ENDPOINT"), os.Getenv("YDB_DATABASE"), false),
		ydbZap.WithTraces(
			logger,
			trace.DetailsAll,
		),
	)
}

func TestMain(m *testing.M) {
	ctx, cancel := context.WithCancel(context.Background())
	defer cancel()
	logger, err := zap.NewDevelopment()
	if err != nil {
		panic(fmt.Errorf("failed to setup logger: %v", err))
	}
	db, err := conn(ctx, logger)
	if err != nil {
		panic(fmt.Errorf("error opening connection for setup: %w", err))
	}
	defer func() { _ = db.Close(ctx) }()

	prefix := path.Join(db.Name(), prefix)

	err = db.Table().Do(
		ctx,
		func(ctx context.Context, s table.Session) error {
			err = s.ExecuteSchemeQuery(ctx, fmt.Sprintf(schema, prefix))
			return err
		},
	)
	if err != nil {
		panic(err)
	}

	os.Exit(m.Run())
}

func TestClientBasic(t *testing.T) {
	ctx, cancel := context.WithCancel(context.Background())
	defer cancel()
	logger := zaptest.NewLogger(t)
	db, err := conn(ctx, logger)
	assert.NoError(t, err, "Cannot open connection to ydb")
	defer func() { _ = db.Close(ctx) }()
	type args struct {
		ctx         context.Context
		description string
		hosts       []string
		hbTimeout   uint64
		lockType    ydblock.HostLockType
	}
	createLockArgs := args{
		ctx:         ctx,
		description: "Test lock",
		hosts: []string{
			"host1.example.com",
			"host2.example.com",
		},
		hbTimeout: 300,
		lockType:  ydblock.HostLockTypeOther,
	}
	want := &ydblock.Lock{
		Hosts: []string{
			"host1.example.com",
			"host2.example.com",
		},
		Description: "Test lock",
		Timeout:     300,
		Type:        ydblock.HostLockTypeOther,
	}
	c := &ydblock.Client{
		DB:     db,
		Root:   root,
		Logger: logger,
	}
	err = c.Init(ctx)
	assert.NoError(t, err)
	got, err := c.CreateLock(createLockArgs.ctx, createLockArgs.description, createLockArgs.hosts, createLockArgs.hbTimeout, createLockArgs.lockType)
	assert.NoError(t, err)
	assert.Greater(t, got.Deadline, createLockArgs.hbTimeout)
	lockWithoutDeadline := *got
	lockWithoutDeadline.Deadline = 0
	assert.Equal(t, want, &lockWithoutDeadline)
	_, err = c.CreateLock(createLockArgs.ctx, createLockArgs.description, createLockArgs.hosts, createLockArgs.hbTimeout, createLockArgs.lockType)
	assert.Error(t, err, "Second CreateLock() didn't return err, but should")
	for _, host := range createLockArgs.hosts {
		err = c.CheckHostLock(ctx, host, got)
		assert.NoErrorf(t, err, "Cannot find lock for host %s: %v", host, err)
	}
	extendedLock, err := c.ExtendLock(ctx, got)
	assert.NoError(t, err)
	assert.Greater(t, got.Deadline+got.Timeout, extendedLock.Deadline)
	err = c.ReleaseLock(ctx, extendedLock)
	assert.NoError(t, err)
	for _, host := range createLockArgs.hosts {
		err = c.CheckHostLock(ctx, host, got)
		assert.Errorf(t, err, "Lock not released for host %s: %v", host, err)
	}
}

func TestClientIntersected(t *testing.T) {
	ctx, cancel := context.WithCancel(context.Background())
	defer cancel()
	logger := zaptest.NewLogger(t)
	db, err := conn(ctx, logger)
	assert.NoError(t, err, "Cannot open connection to ydb")
	defer func() { _ = db.Close(ctx) }()
	c := &ydblock.Client{
		DB:     db,
		Root:   root,
		Logger: logger,
	}
	err = c.Init(ctx)
	assert.NoError(t, err)
	_, err = c.CreateLock(ctx, "Test lock 1", []string{"host3.example.com", "host4.example.com"}, 300, ydblock.HostLockTypeOther)
	assert.NoError(t, err)
	_, err = c.CreateLock(ctx, "Test lock 2", []string{"host4.example.com", "host5.example.com"}, 300, ydblock.HostLockTypeOther)
	assert.ErrorContains(t, err, "some hosts (1) already locked")
	assert.ErrorContains(t, err, "Test lock 1")
}

func TestClientAddHosts(t *testing.T) {
	ctx, cancel := context.WithCancel(context.Background())
	defer cancel()
	logger := zaptest.NewLogger(t)
	db, err := conn(ctx, logger)
	assert.NoError(t, err, "Cannot open connection to ydb")
	defer func() { _ = db.Close(ctx) }()
	c := &ydblock.Client{
		DB:     db,
		Root:   root,
		Logger: logger,
	}
	err = c.Init(ctx)
	assert.NoError(t, err)
	_, err = c.CreateLock(ctx, "Test lock add-hosts", []string{"host6.example.com", "host7.example.com"}, 300, ydblock.HostLockTypeAddHosts)
	assert.NoError(t, err)
	_, err = c.CreateLock(ctx, "Test lock", []string{"host6.example.com", "host7.example.com"}, 300, ydblock.HostLockTypeOther)
	assert.NoError(t, err)
}
