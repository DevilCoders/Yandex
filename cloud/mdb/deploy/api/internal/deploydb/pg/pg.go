package pg

import (
	"context"
	"sync/atomic"
	"time"

	"github.com/jmoiron/sqlx"

	"a.yandex-team.ru/cloud/mdb/deploy/api/internal/deploydb"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/pgutil"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/x/yandex/hasql/tracers"
)

var (
	querySelectLatestMigration = sqlutil.Stmt{
		Name:  "SelectLatestMigration",
		Query: "SELECT max(version) FROM public.schema_version",
	}
)

const (
	targetMigration = 45
)

type backend struct {
	logger  log.Logger
	stopCtx context.Context
	cancel  context.CancelFunc

	cluster       *sqlutil.Cluster
	lastMigration atomic.Value
}

var _ deploydb.Backend = &backend{}

// New constructs PostgreSQL backend
func New(cfg pgutil.Config, l log.Logger) (deploydb.Backend, error) {
	cluster, err := pgutil.NewCluster(cfg, sqlutil.WithTracer(tracers.Log(l)))
	if err != nil {
		return nil, err
	}

	return NewWithCluster(cluster, l), nil
}

// NewWithCluster constructs PostgreSQL backend with custom cluster (useful for mocking)
func NewWithCluster(cluster *sqlutil.Cluster, l log.Logger) deploydb.Backend {
	ctx, cancel := context.WithCancel(context.Background())
	b := &backend{
		logger:  l,
		stopCtx: ctx,
		cancel:  cancel,
		cluster: cluster,
	}

	go b.migrationChecker(ctx, time.Minute, time.Second)
	return b
}

func (b *backend) Close() error {
	b.cancel()
	return b.cluster.Close()
}

func (b *backend) IsReady(ctx context.Context) error {
	node := b.cluster.Alive()
	if node == nil {
		return semerr.Unavailable("unavailable")
	}

	migration := b.lastMigration.Load()
	if migration == nil {
		return xerrors.New("migration not loaded")
	}

	return checkMigration(migration.(int64))
}

func (b *backend) migrationChecker(ctx context.Context, successPeriod, failurePeriod time.Duration) {
	b.logger.Debug("Starting migration checker goroutine")

	timer := time.NewTimer(failurePeriod)

	for {
		select {
		case <-ctx.Done():
			if !timer.Stop() {
				<-timer.C
			}

			b.logger.Debug("Stopping migration checker goroutine")
			return
		case <-timer.C:
			if err := b.loadMigration(ctx); err != nil {
				timer.Reset(failurePeriod)
				continue
			}

			timer.Reset(successPeriod)
		}
	}
}

func (b *backend) loadMigration(ctx context.Context) error {
	migration, err := b.migrationVersion(ctx)
	if err != nil {
		return err
	}

	b.lastMigration.Store(migration)
	return checkMigration(migration)
}

func checkMigration(migration int64) error {
	if migration < targetMigration {
		return xerrors.Errorf("wrong migration version: expected %d or higher, has %d", targetMigration, migration)
	}

	return nil
}

func (b *backend) migrationVersion(ctx context.Context) (int64, error) {
	node := b.cluster.Alive()
	if node == nil {
		return 0, semerr.Unavailable("unavailable")
	}

	var version int64
	parser := func(rows *sqlx.Rows) error {
		return rows.Scan(&version)
	}

	count, err := sqlutil.QueryNode(
		ctx,
		node,
		querySelectLatestMigration,
		nil,
		parser,
		b.logger,
	)
	if err != nil {
		return version, err
	}
	if count == 0 {
		return 0, semerr.NotFound("migration not found")
	}

	return version, nil
}

func (b *backend) Begin(ctx context.Context, ns sqlutil.NodeStateCriteria) (context.Context, error) {
	binding, err := sqlutil.Begin(ctx, b.cluster, ns, nil)
	if err != nil {
		return ctx, err
	}

	return sqlutil.WithTxBinding(ctx, binding), nil
}

func (b *backend) Commit(ctx context.Context) error {
	binding, ok := sqlutil.TxBindingFrom(ctx)
	if !ok {
		return xerrors.New("no transaction found in context")
	}

	return binding.Commit(ctx)
}

func (b *backend) Rollback(ctx context.Context) error {
	binding, ok := sqlutil.TxBindingFrom(ctx)
	if !ok {
		return xerrors.New("no transaction found in context")
	}

	return binding.Rollback(ctx)
}
