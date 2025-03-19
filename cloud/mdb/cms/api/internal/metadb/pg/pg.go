package pg

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/pgutil"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/x/yandex/hasql/tracers"
)

var _ metadb.Backend = &metaDB{}

type metaDB struct {
	l log.Logger

	cluster *sqlutil.Cluster
}

func New(cfg pgutil.Config, l log.Logger) (metadb.Backend, error) {
	cluster, err := pgutil.NewCluster(cfg, sqlutil.WithTracer(tracers.Log(l)))
	if err != nil {
		return nil, err
	}

	return NewWithCluster(cluster, l), nil
}

func NewWithCluster(cluster *sqlutil.Cluster, l log.Logger) metadb.Backend {
	return &metaDB{
		l:       l,
		cluster: cluster,
	}
}

func (mdb *metaDB) Close() error {
	return mdb.cluster.Close()
}

func (mdb *metaDB) IsReady(ctx context.Context) error {
	node := mdb.cluster.Alive()
	if node == nil {
		return semerr.Unavailable("unavailable")
	}

	return nil
}

func (mdb *metaDB) Begin(ctx context.Context, ns sqlutil.NodeStateCriteria) (context.Context, error) {
	binding, err := sqlutil.Begin(ctx, mdb.cluster, ns, nil)
	if err != nil {
		return ctx, err
	}

	return sqlutil.WithTxBinding(ctx, binding), nil
}

func (mdb *metaDB) Commit(ctx context.Context) error {
	binding, ok := sqlutil.TxBindingFrom(ctx)
	if !ok {
		return xerrors.New("no transaction found in context")
	}

	return binding.Commit(ctx)
}

func (mdb *metaDB) Rollback(ctx context.Context) error {
	binding, ok := sqlutil.TxBindingFrom(ctx)
	if !ok {
		return xerrors.New("no transaction found in context")
	}

	return binding.Rollback(ctx)
}
