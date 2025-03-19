package main

import (
	"context"
	"database/sql"
	"os"

	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/misc"

	"go.uber.org/zap"

	"a.yandex-team.ru/cloud/compute/go-common/logging/ctxlog"

	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"

	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/ydbsql"
)

const (
	dbHost = "localhost:2135"
)

var (
	dbName = os.Getenv("DBNAME")
	dbRoot = os.Getenv("DBROOT")
	ydbDB  *sql.DB
)

func init() {
	opts := []ydbsql.ConnectorOption{
		ydbsql.WithEndpoint(dbHost),
		ydbsql.WithDatabase(dbName),
	}

	ydbDB = sql.OpenDB(ydbsql.Connector(opts...))
}

type txFunc func(ctx context.Context, tx *sql.Tx) error

func runInTx(ctx context.Context, name string, f txFunc) (err error) {
	logger := ctxlog.G(ctx).With(zap.String("tx_name", name))
	ctx = ctxlog.WithLogger(ctx, logger)
	tx, err := ydbDB.Begin()
	if err != nil {
		logger.Error("Can't start transaction", zap.Error(err))
		return err
	}

	defer func() {
		if err == nil {
			err = tx.Commit()
			if err != nil {
				logger.Error("Error while commit transaction", zap.Error(err))
			}
		} else {
			logger.Error("Rollback transaction because", zap.Error(err))
			err2 := tx.Rollback()
			if err2 != nil {
				logger.Error("Error rollback transaction", zap.Error(err))
			}
		}
	}()

	return f(ctx, tx)
}

func retryInTx(ctx context.Context, name string, f txFunc) error {
	var retryError error
	_ = misc.Retry(ctx, name, func() error {
		retryError = runInTx(ctx, name, f)
		if retryError == nil {
			return nil
		}
		if kikimrRetryable(retryError) {
			return misc.ErrInternalRetry
		} else {
			return retryError
		}
	})
	return retryError
}

func kikimrRetryable(err error) bool {
	// new ydb driver internal error checker (abort/rollback -> Retrible() == true)
	checker := ydb.RetryChecker{}
	return checker.Check(err).Retriable()
}
