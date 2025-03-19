package sqlutil_test

import (
	"context"
	"testing"

	"github.com/jmoiron/sqlx"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/dbteststeps"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
	"a.yandex-team.ru/library/go/core/xerrors"
)

var (
	queryGetXID = sqlutil.Stmt{
		Name: "GetXID",
		// language=PostgreSQL
		Query: "SELECT txid_current()",
	}

	queryCreateTestTable = sqlutil.Stmt{
		Name: "CreateTableFoo",
		// language=PostgreSQL
		Query: "CREATE TABLE sample_test_table(a text)",
	}

	queryTestTableExists = sqlutil.Stmt{
		Name: "TableFooExists",
		// language=PostgreSQL
		Query: "SELECT count(*) FROM information_schema.tables WHERE table_name='sample_test_table'",
	}
)

type int64Parser struct {
	Value int64
}

func (p *int64Parser) parse(rows *sqlx.Rows) error {
	return rows.Scan(&p.Value)
}

func TestInTxDoQueries(t *testing.T) {
	cluster, _, err := dbteststeps.NewReadyCluster("pg", "postgres")
	require.NoError(t, err)
	L, err := zap.New(zap.KVConfig(log.DebugLevel))
	require.NoError(t, err)

	t.Run("call queries in one transaction", func(t *testing.T) {
		require.NoError(
			t,
			sqlutil.InTxDoQueries(
				context.Background(),
				cluster.Primary(),
				L,
				func(query sqlutil.QueryCallback) error {
					var xid1, xid2 int64Parser
					require.NoError(
						t,
						query(queryGetXID, map[string]interface{}{}, xid1.parse),
					)
					require.NoError(
						t,
						query(queryGetXID, map[string]interface{}{}, xid2.parse),
					)
					require.Equal(t, xid1.Value, xid2.Value, "same XIDs")
					return nil
				},
			),
		)
	})

	t.Run("return errors and do rollback", func(t *testing.T) {
		require.Error(
			t,
			sqlutil.InTxDoQueries(
				context.Background(),
				cluster.Primary(),
				L,
				func(query sqlutil.QueryCallback) error {
					require.NoError(
						t,
						query(queryCreateTestTable, map[string]interface{}{}, sqlutil.NopParser),
					)
					return xerrors.New("some error")
				},
			),
		)

		var tablesCountParser int64Parser

		_, err := sqlutil.QueryContext(
			context.Background(),
			cluster.PrimaryChooser(),
			queryTestTableExists,
			map[string]interface{}{},
			tablesCountParser.parse,
			L,
		)
		require.NoError(t, err)

		require.Equal(t, int64(0), tablesCountParser.Value, "sample_test_table shouldn't be created")
	})
}
