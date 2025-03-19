package usage

import (
	"context"
	"database/sql"
)

type CHDB interface {
	Begin() (*sql.Tx, error)

	ExecContext(ctx context.Context, query string, args ...interface{}) (sql.Result, error)

	SelectContext(ctx context.Context, dest interface{}, query string, args ...interface{}) error
	GetContext(ctx context.Context, dest interface{}, query string, args ...interface{}) error
}

func New(connection CHDB) *Queries {
	return &Queries{
		connection: connection,
	}
}

type Queries struct {
	connection CHDB
}
