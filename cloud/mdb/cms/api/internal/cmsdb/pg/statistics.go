package pg

import (
	"context"
	"time"

	"github.com/jmoiron/sqlx"

	db_models "a.yandex-team.ru/cloud/mdb/cms/api/internal/cmsdb/pg/internal/models"
	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/models"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
)

var (
	queryGetRequestsInWindow = sqlutil.NewStmt(
		"GetRequestsInWindow",
		// language=PostgreSQL
		`
SELECT *
FROM cms.requests r
WHERE r.created_at >= :window_left_border
`,
		db_models.Request{},
	)
	queryGetUnfinishedRequests = sqlutil.NewStmt(
		"GetUnfinishedRequests",
		// language=PostgreSQL
		`
SELECT r.*
FROM cms.requests r
INNER JOIN cms.decisions d ON r.id = d.request_id
WHERE d.status in ('before-done', 'cleanup')
  AND r.is_deleted = true
  AND r.came_back_at < :window_left_border
`,
		db_models.Request{},
	)
	queryGetResetupRequests = sqlutil.NewStmt(
		"GetResetupRequests",
		// language=PostgreSQL
		`
SELECT r.*
FROM cms.requests r
INNER JOIN cms.decisions d ON r.id = d.request_id
WHERE d.status in ('before-done', 'at-wall-e')
  AND r.is_deleted = false
  AND r.resolved_at < :window_left_border
  AND r.name != 'change-disk'
`,
		db_models.Request{},
	)
)

func (b *Backend) GetRequestsStatInWindow(ctx context.Context, window time.Duration) ([]models.ManagementRequest, error) {
	return b.getReqsInWindow(ctx, queryGetRequestsInWindow, window)
}

func (b *Backend) getReqsInWindow(ctx context.Context, query sqlutil.Stmt, window time.Duration) ([]models.ManagementRequest, error) {
	var results []models.ManagementRequest
	parser := func(rows *sqlx.Rows) error {
		var dbReq db_models.Request

		if err := rows.StructScan(&dbReq); err != nil {
			return err
		}
		request, err := dbReq.ToInternal()

		if err != nil {
			return err
		}
		results = append(results, request)
		return nil
	}

	if _, err := sqlutil.QueryContext(
		ctx,
		b.cluster.AliveChooser(),
		query,
		map[string]interface{}{
			"window_left_border": time.Now().Add(-window),
		},
		parser,
		b.log,
	); err != nil {
		return nil, err
	}
	return results, nil
}

func (b *Backend) GetUnfinishedRequests(ctx context.Context, window time.Duration) ([]models.ManagementRequest, error) {
	return b.getReqsInWindow(ctx, queryGetUnfinishedRequests, window)
}

func (b *Backend) GetResetupRequests(ctx context.Context, window time.Duration) ([]models.ManagementRequest, error) {
	return b.getReqsInWindow(ctx, queryGetResetupRequests, window)
}
