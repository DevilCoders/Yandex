package pg

import (
	"context"

	"github.com/jmoiron/sqlx"

	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/sqlerrors"
)

var (
	queryLockCluster = sqlutil.Stmt{
		Name: "LockCluster",
		Query: `
SELECT
	rev
FROM code.lock_cluster(
    i_cid          => :cid,
    i_x_request_id => :x_request_id
)`,
	}

	queryCompleteClusterChange = sqlutil.Stmt{
		Name: "CompleteClusterChange",
		Query: `
SELECT code.complete_cluster_change(
    i_cid => :cid,
    i_rev => :rev
)`,
	}
)

func (mdb *metaDB) LockCluster(ctx context.Context, cid, reqid string) (int64, error) {
	var rev int64
	parser := func(rows *sqlx.Rows) error {
		return rows.Scan(&rev)
	}

	count, err := sqlutil.QueryTx(
		ctx,
		queryLockCluster,
		map[string]interface{}{
			"cid":          cid,
			"x_request_id": reqid,
		},
		parser,
		mdb.l,
	)
	if err != nil {
		return 0, err
	}
	if count == 0 {
		return 0, sqlerrors.ErrNotFound
	}

	return rev, nil
}

func (mdb *metaDB) CompleteClusterChange(ctx context.Context, cid string, revision int64) error {
	_, err := sqlutil.QueryTx(
		ctx,
		queryCompleteClusterChange,
		map[string]interface{}{
			"cid": cid,
			"rev": revision,
		},
		sqlutil.NopParser,
		mdb.l,
	)

	return err
}
