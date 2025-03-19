package scheduler

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/backup/scheduler/internal/models"
	"a.yandex-team.ru/cloud/mdb/internal/ready"
)

type Scheduler interface {
	ready.Checker
	PlanBackups(ctx context.Context, dryrun bool) (models.PlannedStats, error)
	ObsoleteBackups(ctx context.Context, dryrun bool) (models.ObsoleteStats, error)
	PurgeBackups(ctx context.Context, dryrun bool) (models.PurgeStats, error)
}
