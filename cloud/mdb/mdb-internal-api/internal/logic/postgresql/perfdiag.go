package postgresql

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/postgresql/pgmodels"
)

type PerfDiag interface {
	GetSessionsStats(ctx context.Context, cid string, limit int64, opts pgmodels.GetSessionsStatsOptions,
	) (stats []pgmodels.SessionsStats, more bool, err error)
	GetSessionsAtTime(ctx context.Context, cid string, limit int64, opts pgmodels.GetSessionsAtTimeOptions,
	) (stats pgmodels.SessionsAtTime, more bool, err error)
	GetStatementsAtTime(ctx context.Context, cid string, limit int64, opts pgmodels.GetStatementsAtTimeOptions,
	) (stats pgmodels.StatementsAtTime, more bool, err error)
	GetStatementsDiff(ctx context.Context, cid string, limit int64, opts pgmodels.GetStatementsDiffOptions,
	) (stats pgmodels.DiffStatements, more bool, err error)
	GetStatementsInterval(ctx context.Context, cid string, limit int64, opts pgmodels.GetStatementsIntervalOptions,
	) (stats pgmodels.StatementsInterval, more bool, err error)
	GetStatementsStats(ctx context.Context, cid string, limit int64, opts pgmodels.GetStatementsStatsOptions,
	) (stats pgmodels.StatementsStats, more bool, err error)
	GetStatementStats(ctx context.Context, cid string, limit int64, opts pgmodels.GetStatementStatsOptions,
	) (stats pgmodels.StatementStats, more bool, err error)
}
