package pg

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/library/go/core/xerrors"
)

// Begin transaction
func (mdb *metaDB) Begin(ctx context.Context, ns sqlutil.NodeStateCriteria) (context.Context, error) {
	binding, err := sqlutil.Begin(ctx, mdb.cluster, ns, nil)
	if err != nil {
		return ctx, err
	}

	return sqlutil.WithTxBinding(ctx, binding), nil
}

// Commit transaction
func (mdb *metaDB) Commit(ctx context.Context) error {
	binding, ok := sqlutil.TxBindingFrom(ctx)
	if !ok {
		return xerrors.New("no transaction found in context")
	}

	return binding.Commit(ctx)
}

// Rollback transaction
func (mdb *metaDB) Rollback(ctx context.Context) error {
	binding, ok := sqlutil.TxBindingFrom(ctx)
	if !ok {
		return xerrors.New("no transaction found in context")
	}

	return binding.Rollback(ctx)
}
