package pg

import (
	"context"

	"github.com/jmoiron/sqlx"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/cmsdb"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
)

var (
	queryGetLock = sqlutil.Stmt{
		Name: "GetLock",
		// language=PostgreSQL
		Query: `SELECT pg_try_advisory_xact_lock(:key)`,
	}
)

// use only in transaction
func (b *Backend) GetLock(ctx context.Context, key cmsdb.LockKey) error {
	var gotLock bool
	if _, err := sqlutil.QueryTx(
		ctx,
		queryGetLock,
		map[string]interface{}{
			"key": key,
		},
		func(rows *sqlx.Rows) error {
			return rows.Scan(&gotLock)
		},
		b.log,
	); err != nil {
		return err
	}
	if !gotLock {
		b.log.Info("Lock not taken")
		return cmsdb.ErrLockNotTaken
	}
	b.log.Debug("Lock taken")
	return nil
}
