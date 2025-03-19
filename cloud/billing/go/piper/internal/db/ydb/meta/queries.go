package meta

import (
	"context"
	"database/sql"

	"github.com/jmoiron/sqlx"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/db/ydb/qtool"
)

type DBTX interface {
	ExecContext(ctx context.Context, query string, args ...interface{}) (sql.Result, error)

	SelectContext(ctx context.Context, dest interface{}, query string, args ...interface{}) error
	QueryxContext(ctx context.Context, query string, args ...interface{}) (*sqlx.Rows, error)
}

func New(db DBTX, params qtool.QueryParams) *Queries {
	return &Queries{db: db, qp: params}
}

type Queries struct {
	db DBTX
	qp qtool.QueryParams
}
