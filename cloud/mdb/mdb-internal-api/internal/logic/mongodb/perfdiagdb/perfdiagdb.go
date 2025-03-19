package perfdiagdb

import (
	"context"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/ready"
	"a.yandex-team.ru/cloud/mdb/internal/sqlfilter"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/mongodb/mongomodels"
)

//go:generate ../../../../../scripts/mockgen.sh Backend

type Backend interface {
	ready.Checker

	ProfilerStats(
		ctx context.Context,
		cid string,
		limit, offset int64,
		fromTS, toTS time.Time,
		rollupPeriod int64,
		aggregateBy mongomodels.ProfilerStatsColumn,
		aggregationFunction mongomodels.AggregationType,
		groupBy []mongomodels.ProfilerStatsGroupBy,
		topX int64,
		filter []sqlfilter.Term,
	) ([]mongomodels.ProfilerStats, bool, error)

	ProfilerRecs(
		ctx context.Context,
		cid string,
		limit, offset int64,
		fromTS, toTS time.Time,
		requestForm string,
		hostname string,
	) ([]mongomodels.ProfilerRecs, bool, error)

	ProfilerTopForms(
		ctx context.Context,
		cid string,
		limit, offset int64,
		fromTS, toTS time.Time,
		aggregateBy mongomodels.ProfilerStatsColumn,
		aggregationFunction mongomodels.AggregationType,
		filter []sqlfilter.Term,
	) ([]mongomodels.TopForms, bool, error)

	PossibleIndexes(
		ctx context.Context,
		cid string,
		limit, offset int64,
		fromTS, toTS time.Time,
		filter []sqlfilter.Term,
	) ([]mongomodels.PossibleIndexes, bool, error)
}
