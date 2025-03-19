package provider

import (
	"context"
	"time"

	"github.com/jonboulle/clockwork"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/sessions"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/mysql"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/mysql/mymodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/mysql/perfdiagdb"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
)

type PerfDiag struct {
	cfg      logic.Config
	sessions sessions.Sessions
	mypddb   perfdiagdb.Backend
	clock    clockwork.Clock
}

var _ mysql.PerfDiag = &PerfDiag{}

func NewPerfDiag(cfg logic.Config, sessions sessions.Sessions, pgdb perfdiagdb.Backend, clock clockwork.Clock) *PerfDiag {
	return &PerfDiag{cfg: cfg, sessions: sessions, mypddb: pgdb, clock: clock}
}

func (pd *PerfDiag) GetSessionsStats(ctx context.Context, cid string, limit int64, opts mymodels.GetSessionsStatsOptions) ([]mymodels.SessionsStats, bool, error) {
	if pd.mypddb == nil {
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
		opts.GroupBy = []mymodels.MySessionsColumn{mymodels.MySessionsStage, mymodels.MySessionsUser}
	}

	// Set default values for 'from' and 'to' if needed
	if !opts.FromTS.Valid {
		opts.FromTS = optional.NewTime(defaultFromTS(pd.clock))
	}
	if !opts.ToTS.Valid {
		opts.ToTS = optional.NewTime(defaultToTS(pd.clock))
	}

	return pd.mypddb.SessionsStats(ctx, cid, limit, opts.Offset.Int64, opts.FromTS.Time, opts.ToTS.Time, opts.RollupPeriod.Int64, opts.GroupBy, opts.OrderBy, opts.Filter)

}

func (pd *PerfDiag) GetSessionsAtTime(ctx context.Context, cid string, limit int64, opts mymodels.GetSessionsAtTimeOptions) (mymodels.SessionsAtTime, bool, error) {
	if pd.mypddb == nil {
		return mymodels.SessionsAtTime{}, false, semerr.NotImplemented("not implemented")
	}

	ctx, _, err := pd.sessions.Begin(ctx, sessions.ResolveByCluster(cid, models.PermMDBAllRead))
	if err != nil {
		return mymodels.SessionsAtTime{}, false, err
	}
	defer pd.sessions.Rollback(ctx)
	if limit <= 0 {
		limit = 100
	}
	if opts.Offset.Int64 < 0 {
		opts.Offset.Int64 = 0
	}

	return pd.mypddb.SessionsAtTime(ctx, cid, limit, opts.Offset.Int64, opts.Time, opts.ColumnFilter, opts.Filter, opts.OrderBy)

}

func (pd *PerfDiag) GetStatementsAtTime(ctx context.Context, cid string, limit int64, opts mymodels.GetStatementsAtTimeOptions) (mymodels.StatementsAtTime, bool, error) {
	if pd.mypddb == nil {
		return mymodels.StatementsAtTime{}, false, semerr.NotImplemented("not implemented")
	}

	ctx, _, err := pd.sessions.Begin(ctx, sessions.ResolveByCluster(cid, models.PermMDBAllRead))
	if err != nil {
		return mymodels.StatementsAtTime{}, false, err
	}
	defer pd.sessions.Rollback(ctx)
	err = opts.ValidateOrderBy()
	if err != nil {
		return mymodels.StatementsAtTime{}, false, err
	}
	if limit <= 0 {
		limit = 100
	}
	if opts.Offset.Int64 < 0 {
		opts.Offset.Int64 = 0
	}

	return pd.mypddb.StatementsAtTime(ctx, cid, limit, opts.Offset.Int64, opts.Time, opts.ColumnFilter, opts.Filter, opts.OrderBy)

}

func (pd *PerfDiag) GetStatementsDiff(ctx context.Context, cid string, limit int64, opts mymodels.GetStatementsDiffOptions) (mymodels.DiffStatements, bool, error) {
	if pd.mypddb == nil {
		return mymodels.DiffStatements{}, false, semerr.NotImplemented("not implemented")
	}

	ctx, _, err := pd.sessions.Begin(ctx, sessions.ResolveByCluster(cid, models.PermMDBAllRead))
	if err != nil {
		return mymodels.DiffStatements{}, false, err
	}
	defer pd.sessions.Rollback(ctx)
	err = opts.ValidateOrderBy()
	if err != nil {
		return mymodels.DiffStatements{}, false, err
	}

	if limit <= 0 {
		limit = 100
	}
	if opts.Offset.Int64 < 0 {
		opts.Offset.Int64 = 0
	}

	return pd.mypddb.StatementsDiff(ctx, cid, limit, opts.Offset.Int64, opts.FirstIntervalStart, opts.SecondIntervalStart, opts.IntervalsDuration.Int64, opts.ColumnFilter, opts.Filter, opts.OrderBy)

}

func (pd *PerfDiag) GetStatementsInterval(ctx context.Context, cid string, limit int64, opts mymodels.GetStatementsIntervalOptions) (mymodels.StatementsInterval, bool, error) {
	if pd.mypddb == nil {
		return mymodels.StatementsInterval{}, false, semerr.NotImplemented("not implemented")
	}

	ctx, _, err := pd.sessions.Begin(ctx, sessions.ResolveByCluster(cid, models.PermMDBAllRead))
	if err != nil {
		return mymodels.StatementsInterval{}, false, err
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

	return pd.mypddb.StatementsInterval(ctx, cid, limit, opts.Offset.Int64, opts.FromTS.Time, opts.ToTS.Time, opts.ColumnFilter, opts.Filter, opts.OrderBy)

}

func (pd *PerfDiag) GetStatementsStats(ctx context.Context, cid string, limit int64, opts mymodels.GetStatementsStatsOptions) (mymodels.StatementsStats, bool, error) {
	if pd.mypddb == nil {
		return mymodels.StatementsStats{}, false, semerr.NotImplemented("not implemented")
	}
	ctx, _, err := pd.sessions.Begin(ctx, sessions.ResolveByCluster(cid, models.PermMDBAllRead))
	if err != nil {
		return mymodels.StatementsStats{}, false, err
	}
	if limit <= 0 {
		limit = 100
	}
	if opts.Offset.Int64 < 0 {
		opts.Offset.Int64 = 0
	}

	//if len(opts.GroupBy) == 0 {
	//	opts.GroupBy = []mymodels.StatementsStatsGroupBy{mymodels.StatementsStatsGroupByUser}
	//}

	// Set default values for 'from' and 'to' if needed
	if !opts.FromTS.Valid {
		opts.FromTS = optional.NewTime(defaultFromTS(pd.clock))
	}
	if !opts.ToTS.Valid {
		opts.ToTS = optional.NewTime(defaultToTS(pd.clock))
	}

	return pd.mypddb.StatementsStats(ctx, cid, limit, opts.Offset.Int64, opts.FromTS.Time, opts.ToTS.Time, opts.GroupBy, opts.Filter, opts.ColumnFilter)
}

func (pd *PerfDiag) GetStatementStats(ctx context.Context, cid string, limit int64, opts mymodels.GetStatementStatsOptions) (mymodels.StatementStats, bool, error) {
	if pd.mypddb == nil {
		return mymodels.StatementStats{}, false, semerr.NotImplemented("not implemented")
	}
	ctx, _, err := pd.sessions.Begin(ctx, sessions.ResolveByCluster(cid, models.PermMDBAllRead))
	if err != nil {
		return mymodels.StatementStats{}, false, err
	}
	if limit <= 0 {
		limit = 100
	}
	if opts.Offset.Int64 < 0 {
		opts.Offset.Int64 = 0
	}

	if opts.Digest == "" {
		return mymodels.StatementStats{}, false, semerr.InvalidInput("digest is required")
	}
	// Set default values for 'from' and 'to' if needed
	if !opts.FromTS.Valid {
		opts.FromTS = optional.NewTime(defaultFromTS(pd.clock))
	}
	if !opts.ToTS.Valid {
		opts.ToTS = optional.NewTime(defaultToTS(pd.clock))
	}

	return pd.mypddb.StatementStats(ctx, cid, opts.Digest, limit, opts.Offset.Int64, opts.FromTS.Time, opts.ToTS.Time, opts.GroupBy, opts.Filter, opts.ColumnFilter)
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
