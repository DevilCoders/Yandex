package mysql

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/mysql/mymodels"
)

type PerfDiag interface {
	GetSessionsStats(ctx context.Context, cid string, limit int64, opts mymodels.GetSessionsStatsOptions,
	) (stats []mymodels.SessionsStats, more bool, err error)
	GetSessionsAtTime(ctx context.Context, cid string, limit int64, opts mymodels.GetSessionsAtTimeOptions,
	) (stats mymodels.SessionsAtTime, more bool, err error)
	GetStatementsAtTime(ctx context.Context, cid string, limit int64, opts mymodels.GetStatementsAtTimeOptions,
	) (stats mymodels.StatementsAtTime, more bool, err error)
	GetStatementsDiff(ctx context.Context, cid string, limit int64, opts mymodels.GetStatementsDiffOptions,
	) (stats mymodels.DiffStatements, more bool, err error)
	GetStatementsInterval(ctx context.Context, cid string, limit int64, opts mymodels.GetStatementsIntervalOptions,
	) (stats mymodels.StatementsInterval, more bool, err error)
	GetStatementsStats(ctx context.Context, cid string, limit int64, opts mymodels.GetStatementsStatsOptions,
	) (stats mymodels.StatementsStats, more bool, err error)
	GetStatementStats(ctx context.Context, cid string, limit int64, opts mymodels.GetStatementStatsOptions,
	) (stats mymodels.StatementStats, more bool, err error)
}
