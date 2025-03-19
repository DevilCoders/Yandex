package logsdb

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/internal/ready"
	"a.yandex-team.ru/cloud/mdb/internal/sqlfilter"
	"a.yandex-team.ru/cloud/mdb/logs-api/internal/models"
)

//go:generate ../../../scripts/mockgen.sh Backend

type Backend interface {
	ready.Checker

	Logs(ctx context.Context, criteria Criteria) (logs []models.Log, more bool, err error)
}

type Criteria struct {
	Sources     []models.LogSource
	FromSeconds int64
	FromMS      int64
	ToSeconds   int64
	ToMS        int64
	Order       models.SortOrder
	Offset      int64
	Limit       int64
	Levels      []models.LogLevel
	Filters     []sqlfilter.Term
}
