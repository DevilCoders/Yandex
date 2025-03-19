package provider

import (
	"context"
	"time"

	"github.com/jonboulle/clockwork"

	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/sessions"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/mongodb"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/mongodb/mongomodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/mongodb/perfdiagdb"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
)

type PerfDiag struct {
	cfg      logic.Config
	sessions sessions.Sessions
	perfdb   perfdiagdb.Backend
	clock    clockwork.Clock
}

var _ mongodb.PerfDiag = &PerfDiag{}

func NewPerfDiag(cfg logic.Config, sessions sessions.Sessions, perfdb perfdiagdb.Backend, clock clockwork.Clock) *PerfDiag {
	return &PerfDiag{cfg: cfg, sessions: sessions, perfdb: perfdb, clock: clock}
}

func (pd *PerfDiag) GetProfilerStats(ctx context.Context, cid string, opts mongomodels.GetProfilerStatsOptions) ([]mongomodels.ProfilerStats, bool, error) {
	if pd.perfdb == nil {
		return nil, false, semerr.NotImplemented("not implemented")
	}
	ctx, _, err := pd.sessions.Begin(ctx, sessions.ResolveByCluster(cid, models.PermMDBAllRead))
	if err != nil {
		return nil, false, err
	}
	defer pd.sessions.Rollback(ctx)

	if opts.Limit <= 0 {
		opts.Limit = 100
	}
	if opts.Offset.Int64 < 0 {
		opts.Offset.Int64 = 0
	}

	if opts.RollupPeriod.Int64 < 1 {
		opts.RollupPeriod.Int64 = 1
	}

	if len(opts.GroupBy) == 0 {
		opts.GroupBy = []mongomodels.ProfilerStatsGroupBy{mongomodels.ProfilerStatsGroupByForm}
	}

	if len(opts.AggregateBy) == 0 {
		opts.AggregateBy = mongomodels.ProfilerStatsColumnDuration
	}
	if len(opts.AggregationFunction) == 0 {
		opts.AggregationFunction = mongomodels.AggregationTypeSum
	}

	var topX int64
	if opts.TopX.Valid && (opts.TopX.Int64 >= 1) {
		topX = opts.TopX.Int64
	} else {
		topX = 10
	}

	return pd.perfdb.ProfilerStats(ctx, cid, opts.Limit, opts.Offset.Int64, opts.FromTS, opts.ToTS, opts.RollupPeriod.Int64, opts.AggregateBy, opts.AggregationFunction, opts.GroupBy, topX, opts.Filter)
}

func (pd *PerfDiag) GetProfilerRecsAtTime(ctx context.Context, cid string, opts mongomodels.GetProfilerRecsAtTimeOptions) ([]mongomodels.ProfilerRecs, bool, error) {
	if pd.perfdb == nil {
		return nil, false, semerr.NotImplemented("not implemented")
	}
	ctx, _, err := pd.sessions.Begin(ctx, sessions.ResolveByCluster(cid, models.PermMDBAllRead))
	if err != nil {
		return nil, false, err
	}
	defer pd.sessions.Rollback(ctx)

	if opts.Limit <= 0 {
		opts.Limit = 100
	}
	if opts.Offset.Int64 < 0 {
		opts.Offset.Int64 = 0
	}

	return pd.perfdb.ProfilerRecs(ctx, cid, opts.Limit, opts.Offset.Int64, opts.FromTS, opts.ToTS, opts.RequestForm, opts.Hostname)
}

func (pd *PerfDiag) GetProfilerTopFormsByStat(ctx context.Context, cid string, opts mongomodels.GetProfilerTopFormsByStatOptions) ([]mongomodels.TopForms, bool, error) {
	if pd.perfdb == nil {
		return nil, false, semerr.NotImplemented("not implemented")
	}
	ctx, _, err := pd.sessions.Begin(ctx, sessions.ResolveByCluster(cid, models.PermMDBAllRead))
	if err != nil {
		return nil, false, err
	}
	defer pd.sessions.Rollback(ctx)

	if opts.Limit <= 0 {
		opts.Limit = 100
	}
	if opts.Offset.Int64 < 0 {
		opts.Offset.Int64 = 0
	}

	if len(opts.AggregateBy) == 0 {
		opts.AggregateBy = mongomodels.ProfilerStatsColumnDuration
	}
	if len(opts.AggregationFunction) == 0 {
		opts.AggregationFunction = mongomodels.AggregationTypeSum
	}

	return pd.perfdb.ProfilerTopForms(ctx, cid, opts.Limit, opts.Offset.Int64, opts.FromTS, opts.ToTS, opts.AggregateBy, opts.AggregationFunction, opts.Filter)
}

func (pd *PerfDiag) GetPossibleIndexes(ctx context.Context, cid string, opts mongomodels.GetPossibleIndexesOptions) ([]mongomodels.PossibleIndexes, bool, error) {
	if pd.perfdb == nil {
		return nil, false, semerr.NotImplemented("not implemented")
	}
	ctx, _, err := pd.sessions.Begin(ctx, sessions.ResolveByCluster(cid, models.PermMDBAllRead))
	if err != nil {
		return nil, false, err
	}
	defer pd.sessions.Rollback(ctx)

	if opts.Limit <= 0 {
		opts.Limit = 100
	}
	if opts.Offset.Int64 < 0 {
		opts.Offset.Int64 = 0
	}

	return pd.perfdb.PossibleIndexes(ctx, cid, opts.Limit, opts.Offset.Int64, opts.FromTS, opts.ToTS, opts.Filter)
}

func (pd *PerfDiag) GetValidOperations(ctx context.Context, cid string) ([]mongomodels.ValidOperation, error) {
	if pd.perfdb == nil {
		return nil, semerr.NotImplemented("not implemented")
	}
	ctx, _, err := pd.sessions.Begin(ctx, sessions.ResolveByCluster(cid, models.PermMDBAllRead))
	if err != nil {
		return nil, err
	}
	defer pd.sessions.Rollback(ctx)

	var validOps = []mongomodels.ValidOperation{
		"command",
		"count",
		"getmore",
		"insert",
		"killcursors",
		"query",
		"remove",
		"update",
	}

	return validOps, nil

}

const (
	defaultFromTSModifier = -time.Hour
)

func defaultFromTS(clock clockwork.Clock) time.Time {
	return clock.Now().Add(defaultFromTSModifier).UTC()
}

func defaultToTS(clock clockwork.Clock) time.Time {
	return clock.Now().UTC()
}
