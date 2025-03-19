package storage

import (
	"context"

	"github.com/jmoiron/sqlx"

	"a.yandex-team.ru/transfer_manager/go/pkg/contextutil"
)

var (
	txCtxKey = contextutil.NewContextKey()
)

func withTx(ctx context.Context, tx *sqlx.Tx) context.Context {
	return context.WithValue(ctx, txCtxKey, tx)
}

func getTx(ctx context.Context) *sqlx.Tx {
	value := ctx.Value(txCtxKey)
	if value != nil {
		return value.(*sqlx.Tx)
	}
	return nil
}
