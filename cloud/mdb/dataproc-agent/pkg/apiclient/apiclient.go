package apiclient

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/dataproc-agent/pkg/models"
)

//go:generate ../../../scripts/mockgen.sh HealthAPI,JobStream,JobsAPI

// HealthAPI defines dataproc manager interface for health reporting
type HealthAPI interface {
	// Report sends health report
	Report(ctx context.Context, info models.Info) (*models.NodesToDecommission, error)
}

type JobStream interface {
	Next(ctx context.Context) (*models.Job, error)
}

// JobsAPI defines dataproc manager interface for job management and tracking
type JobsAPI interface {
	ListActiveJobs() JobStream
	UpdateState(context.Context, models.Job) error
}

// Client defines dataproc manager interface to be used by dataproc agent
type Client interface {
	HealthAPI
	JobsAPI
}
