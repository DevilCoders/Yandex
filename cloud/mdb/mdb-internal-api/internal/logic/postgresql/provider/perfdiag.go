package provider

import (
	"context"
	"time"

	"github.com/jonboulle/clockwork"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/sessions"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/postgresql"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/postgresql/perfdiagdb"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/postgresql/pgmodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
)

type PerfDiag struct {
	cfg      logic.Config
	sessions sessions.Sessions
	pgdb     perfdiagdb.Backend
	clock    clockwork.Clock
}

var _ postgresql.PerfDiag = &PerfDiag{}

func NewPerfDiag(cfg logic.Config, sessions sessions.Sessions, pgdb perfdiagdb.Backend, clock clockwork.Clock) *PerfDiag {
	return &PerfDiag{cfg: cfg, sessions: sessions, pgdb: pgdb, clock: clock}
}

func (pd *PerfDiag) GetSessionsStats(ctx context.Context, cid string, limit int64, opts pgmodels.GetSessionsStatsOptions) ([]pgmodels.SessionsStats, bool, error) {
	if pd.pgdb == nil {
		return nil, false, semerr.NotImplemented("not implemented")
	}
	ctx, _, err := pd.sessions.Begin(ctx, sessions.ResolveByCluster(cid, models.PermMDBAllRead))
	if err != nil {
		return nil, false, err
	}
	defer pd.sessions.Rollback(ctx)
	err = opts.ValidateOrderBy()
	if err != nil {
		return nil, false, err
	}
	if limit <= 0 {
		limit = 100
	}
	if opts.Offset.Int64 < 0 {
		opts.Offset.Int64 = 0
	}

	if opts.RollupPeriod.Int64 < 1 {
		opts.RollupPeriod.Int64 = 1
	}

	if len(opts.GroupBy) == 0 {
		opts.GroupBy = []pgmodels.PGStatActivityColumn{pgmodels.PGStatActivityWaitEvent, pgmodels.PGStatActivityUser}
	}

	// Set default values for 'from' and 'to' if needed
	if !opts.FromTS.Valid {
		opts.FromTS = optional.NewTime(defaultFromTS(pd.clock))
	}
	if !opts.ToTS.Valid {
		opts.ToTS = optional.NewTime(defaultToTS(pd.clock))
	}

	return pd.pgdb.SessionsStats(ctx, cid, limit, opts.Offset.Int64, opts.FromTS.Time, opts.ToTS.Time, opts.RollupPeriod.Int64, opts.GroupBy, opts.OrderBy, opts.Filter)

}

func (pd *PerfDiag) GetSessionsAtTime(ctx context.Context, cid string, limit int64, opts pgmodels.GetSessionsAtTimeOptions) (pgmodels.SessionsAtTime, bool, error) {
	if pd.pgdb == nil {
		return pgmodels.SessionsAtTime{}, false, semerr.NotImplemented("not implemented")
	}

	ctx, _, err := pd.sessions.Begin(ctx, sessions.ResolveByCluster(cid, models.PermMDBAllRead))
	if err != nil {
		return pgmodels.SessionsAtTime{}, false, err
	}
	defer pd.sessions.Rollback(ctx)
	if limit <= 0 {
		limit = 100
	}
	if opts.Offset.Int64 < 0 {
		opts.Offset.Int64 = 0
	}

	return pd.pgdb.SessionsAtTime(ctx, cid, limit, opts.Offset.Int64, opts.Time, opts.ColumnFilter, opts.Filter, opts.OrderBy)

}

func (pd *PerfDiag) GetStatementsAtTime(ctx context.Context, cid string, limit int64, opts pgmodels.GetStatementsAtTimeOptions) (pgmodels.StatementsAtTime, bool, error) {
	if pd.pgdb == nil {
		return pgmodels.StatementsAtTime{}, false, semerr.NotImplemented("not implemented")
	}

	ctx, _, err := pd.sessions.Begin(ctx, sessions.ResolveByCluster(cid, models.PermMDBAllRead))
	if err != nil {
		return pgmodels.StatementsAtTime{}, false, err
	}
	defer pd.sessions.Rollback(ctx)
	err = opts.ValidateOrderBy()
	if err != nil {
		return pgmodels.StatementsAtTime{}, false, err
	}
	if limit <= 0 {
		limit = 100
	}
	if opts.Offset.Int64 < 0 {
		opts.Offset.Int64 = 0
	}

	return pd.pgdb.StatementsAtTime(ctx, cid, limit, opts.Offset.Int64, opts.Time, opts.ColumnFilter, opts.Filter, opts.OrderBy)

}

func (pd *PerfDiag) GetStatementsDiff(ctx context.Context, cid string, limit int64, opts pgmodels.GetStatementsDiffOptions) (pgmodels.DiffStatements, bool, error) {
	if pd.pgdb == nil {
		return pgmodels.DiffStatements{}, false, semerr.NotImplemented("not implemented")
	}

	ctx, _, err := pd.sessions.Begin(ctx, sessions.ResolveByCluster(cid, models.PermMDBAllRead))
	if err != nil {
		return pgmodels.DiffStatements{}, false, err
	}
	defer pd.sessions.Rollback(ctx)
	err = opts.ValidateOrderBy()
	if err != nil {
		return pgmodels.DiffStatements{}, false, err
	}

	if limit <= 0 {
		limit = 100
	}
	if opts.Offset.Int64 < 0 {
		opts.Offset.Int64 = 0
	}

	return pd.pgdb.StatementsDiff(ctx, cid, limit, opts.Offset.Int64, opts.FirstIntervalStart, opts.SecondIntervalStart, opts.IntervalsDuration.Int64, opts.ColumnFilter, opts.Filter, opts.OrderBy)

}

func (pd *PerfDiag) GetStatementsInterval(ctx context.Context, cid string, limit int64, opts pgmodels.GetStatementsIntervalOptions) (pgmodels.StatementsInterval, bool, error) {
	if pd.pgdb == nil {
		return pgmodels.StatementsInterval{}, false, semerr.NotImplemented("not implemented")
	}

	ctx, _, err := pd.sessions.Begin(ctx, sessions.ResolveByCluster(cid, models.PermMDBAllRead))
	if err != nil {
		return pgmodels.StatementsInterval{}, false, err
	}
	defer pd.sessions.Rollback(ctx)
	if limit <= 0 {
		limit = 100
	}
	if opts.Offset.Int64 < 0 {
		opts.Offset.Int64 = 0
	}

	// Set default values for 'from' and 'to' if needed
	if !opts.FromTS.Valid {
		opts.FromTS = optional.NewTime(defaultFromTS(pd.clock))
	}
	if !opts.ToTS.Valid {
		opts.ToTS = optional.NewTime(defaultToTS(pd.clock))
	}

	return pd.pgdb.StatementsInterval(ctx, cid, limit, opts.Offset.Int64, opts.FromTS.Time, opts.ToTS.Time, opts.ColumnFilter, opts.Filter, opts.OrderBy)

}

func (pd *PerfDiag) GetStatementsStats(ctx context.Context, cid string, limit int64, opts pgmodels.GetStatementsStatsOptions) (pgmodels.StatementsStats, bool, error) {
	if pd.pgdb == nil {
		return pgmodels.StatementsStats{}, false, semerr.NotImplemented("not implemented")
	}
	ctx, _, err := pd.sessions.Begin(ctx, sessions.ResolveByCluster(cid, models.PermMDBAllRead))
	if err != nil {
		return pgmodels.StatementsStats{}, false, err
	}
	if limit <= 0 {
		limit = 100
	}
	if opts.Offset.Int64 < 0 {
		opts.Offset.Int64 = 0
	}

	if len(opts.GroupBy) == 0 {
		opts.GroupBy = []pgmodels.StatementsStatsGroupBy{pgmodels.StatementsStatsGroupByUser}
	}

	// Set default values for 'from' and 'to' if needed
	if !opts.FromTS.Valid {
		opts.FromTS = optional.NewTime(defaultFromTS(pd.clock))
	}
	if !opts.ToTS.Valid {
		opts.ToTS = optional.NewTime(defaultToTS(pd.clock))
	}

	return pd.pgdb.StatementsStats(ctx, cid, limit, opts.Offset.Int64, opts.FromTS.Time, opts.ToTS.Time, opts.GroupBy, opts.Filter, opts.ColumnFilter)

}

func (pd *PerfDiag) GetStatementStats(ctx context.Context, cid string, limit int64, opts pgmodels.GetStatementStatsOptions) (pgmodels.StatementStats, bool, error) {
	if pd.pgdb == nil {
		return pgmodels.StatementStats{}, false, semerr.NotImplemented("not implemented")
	}
	ctx, _, err := pd.sessions.Begin(ctx, sessions.ResolveByCluster(cid, models.PermMDBAllRead))
	if err != nil {
		return pgmodels.StatementStats{}, false, err
	}
	if limit <= 0 {
		limit = 100
	}
	if opts.Offset.Int64 < 0 {
		opts.Offset.Int64 = 0
	}

	// Set default values for 'from' and 'to' if needed
	if !opts.FromTS.Valid {
		opts.FromTS = optional.NewTime(defaultFromTS(pd.clock))
	}
	if !opts.ToTS.Valid {
		opts.ToTS = optional.NewTime(defaultToTS(pd.clock))
	}

	return pd.pgdb.StatementStats(ctx, cid, opts.Queryid, limit, opts.Offset.Int64, opts.FromTS.Time, opts.ToTS.Time, opts.GroupBy, opts.Filter, opts.ColumnFilter)
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
