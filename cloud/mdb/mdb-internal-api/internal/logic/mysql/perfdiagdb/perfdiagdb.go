package perfdiagdb

import (
	"context"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/ready"
	"a.yandex-team.ru/cloud/mdb/internal/sqlfilter"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/mysql/mymodels"
)

//go:generate ../../../../../scripts/mockgen.sh Backend

type Backend interface {
	ready.Checker

	SessionsStats(
		ctx context.Context,
		cid string,
		limit, offset int64,
		fromTS, toTS time.Time,
		rollupPeriod int64,
		groupBy []mymodels.MySessionsColumn,
		orderBy []mymodels.OrderBy,
		filter []sqlfilter.Term,
	) (response []mymodels.SessionsStats, more bool, err error)

	SessionsAtTime(
		ctx context.Context,
		cid string,
		limit, offset int64,
		ts time.Time,
		columnFilter []mymodels.MySessionsColumn,
		filter []sqlfilter.Term,
		orderBy []mymodels.OrderBySessionsAtTime,
	) (response mymodels.SessionsAtTime, more bool, err error)

	StatementsAtTime(
		ctx context.Context,
		cid string,
		limit, offset int64,
		ts time.Time,
		columnFilter []mymodels.MyStatementsColumn,
		filter []sqlfilter.Term,
		orderBy []mymodels.OrderByStatementsAtTime,
	) (response mymodels.StatementsAtTime, more bool, err error)

	StatementsDiff(
		ctx context.Context,
		cid string,
		limit, offset int64,
		firstIntervalStart, SecondIntervalStart time.Time,
		intervalsDuration int64,
		columnFilter []mymodels.MyStatementsColumn,
		filter []sqlfilter.Term,
		orderBy []mymodels.OrderByStatementsAtTime,
	) (response mymodels.DiffStatements, more bool, err error)

	StatementsInterval(
		ctx context.Context,
		cid string,
		limit, offset int64,
		fromTS, toTS time.Time,
		columnFilter []mymodels.MyStatementsColumn,
		filter []sqlfilter.Term,
		orderBy []mymodels.OrderByStatementsAtTime,
	) (response mymodels.StatementsInterval, more bool, err error)

	StatementsStats(
		ctx context.Context,
		cid string,
		limit, offset int64,
		fromTS, toTS time.Time,
		groupBy []mymodels.StatementsStatsGroupBy,
		filter []sqlfilter.Term,
		columnFilter []mymodels.MyStatementsColumn,
	) (response mymodels.StatementsStats, more bool, err error)

	StatementStats(
		ctx context.Context,
		cid string,
		digest string,
		limit, offset int64,
		fromTS, toTS time.Time,
		groupBy []mymodels.StatementsStatsGroupBy,
		filter []sqlfilter.Term,
		columnFilter []mymodels.MyStatementsColumn,
	) (response mymodels.StatementStats, more bool, err error)
}
