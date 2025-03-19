package uniq

import (
	"context"
	"database/sql"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/db/ydb/qtool"
)

type DBTX interface {
	ExecContext(ctx context.Context, query string, args ...interface{}) (sql.Result, error)

	SelectContext(ctx context.Context, dest interface{}, query string, args ...interface{}) error
	GetContext(ctx context.Context, dest interface{}, query string, args ...interface{}) error
}

func New(db DBTX, params qtool.QueryParams) *Queries {
	return &Queries{db: db, qp: params}
}

type Queries struct {
	db DBTX
	qp qtool.QueryParams
}
