package mongodb

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/mongodb/mongomodels"
)

type PerfDiag interface {
	GetProfilerStats(ctx context.Context, cid string, opts mongomodels.GetProfilerStatsOptions,
	) ([]mongomodels.ProfilerStats, bool, error)
	GetProfilerRecsAtTime(ctx context.Context, cid string, opts mongomodels.GetProfilerRecsAtTimeOptions,
	) ([]mongomodels.ProfilerRecs, bool, error)
	GetProfilerTopFormsByStat(ctx context.Context, cid string, opts mongomodels.GetProfilerTopFormsByStatOptions,
	) ([]mongomodels.TopForms, bool, error)
	GetPossibleIndexes(ctx context.Context, cid string, opts mongomodels.GetPossibleIndexesOptions,
	) ([]mongomodels.PossibleIndexes, bool, error)
	GetValidOperations(ctx context.Context, cid string,
	) ([]mongomodels.ValidOperation, error)
}
