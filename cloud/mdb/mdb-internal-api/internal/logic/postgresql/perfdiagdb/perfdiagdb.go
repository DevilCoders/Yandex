package perfdiagdb

import (
	"context"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/ready"
	"a.yandex-team.ru/cloud/mdb/internal/sqlfilter"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/postgresql/pgmodels"
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
		groupBy []pgmodels.PGStatActivityColumn,
		orderBy []pgmodels.OrderBy,
		filter []sqlfilter.Term,
	) (response []pgmodels.SessionsStats, more bool, err error)

	SessionsAtTime(
		ctx context.Context,
		cid string,
		limit, offset int64,
		ts time.Time,
		columnFilter []pgmodels.PGStatActivityColumn,
		filter []sqlfilter.Term,
		orderBy []pgmodels.OrderBySessionsAtTime,
	) (response pgmodels.SessionsAtTime, more bool, err error)

	StatementsAtTime(
		ctx context.Context,
		cid string,
		limit, offset int64,
		ts time.Time,
		columnFilter []pgmodels.PGStatStatementsColumn,
		filter []sqlfilter.Term,
		orderBy []pgmodels.OrderByStatementsAtTime,
	) (response pgmodels.StatementsAtTime, more bool, err error)

	StatementsDiff(
		ctx context.Context,
		cid string,
		limit, offset int64,
		firstIntervalStart, SecondIntervalStart time.Time,
		intervalsDuration int64,
		columnFilter []pgmodels.PGStatStatementsColumn,
		filter []sqlfilter.Term,
		orderBy []pgmodels.OrderByStatementsAtTime,
	) (response pgmodels.DiffStatements, more bool, err error)

	StatementsInterval(
		ctx context.Context,
		cid string,
		limit, offset int64,
		fromTS, toTS time.Time,
		columnFilter []pgmodels.PGStatStatementsColumn,
		filter []sqlfilter.Term,
		orderBy []pgmodels.OrderByStatementsAtTime,
	) (response pgmodels.StatementsInterval, more bool, err error)

	StatementsStats(
		ctx context.Context,
		cid string,
		limit, offset int64,
		fromTS, toTS time.Time,
		groupBy []pgmodels.StatementsStatsGroupBy,
		filter []sqlfilter.Term,
		columnFilter []pgmodels.StatementsStatsField,
	) (response pgmodels.StatementsStats, more bool, err error)

	StatementStats(
		ctx context.Context,
		cid string,
		queryid string,
		limit, offset int64,
		fromTS, toTS time.Time,
		groupBy []pgmodels.StatementsStatsGroupBy,
		filter []sqlfilter.Term,
		columnFilter []pgmodels.StatementsStatsField,
	) (response pgmodels.StatementStats, more bool, err error)
}
