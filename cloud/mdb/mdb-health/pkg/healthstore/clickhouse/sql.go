package clickhouse

import "a.yandex-team.ru/cloud/mdb/internal/sqlutil"

var (
	queryStoreHostsHealth = sqlutil.Stmt{
		Name:  "InsertHostHealth",
		Query: `INSERT INTO health.host_health`,
	}
)
