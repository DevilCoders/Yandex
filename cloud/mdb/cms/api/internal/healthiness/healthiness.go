package healthiness

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/models"
)

//go:generate ../../../../scripts/mockgen.sh Healthiness

type Healthiness interface {
	ByInstances(ctx context.Context, instances []models.Instance) (res HealthCheckResult, err error)
}
