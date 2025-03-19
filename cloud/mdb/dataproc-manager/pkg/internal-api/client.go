package client

import (
	"context"

	intapi "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/dataproc/v1"
	"a.yandex-team.ru/cloud/mdb/dataproc-manager/pkg/models"
)

//go:generate ../../../scripts/mockgen.sh InternalAPI

// InternalAPI describes internal api client
type InternalAPI interface {
	GetClusterTopology(ctx context.Context, cid string) (models.ClusterTopology, error)

	ListClusterJobs(ctx context.Context, in *intapi.ListJobsRequest) (*intapi.ListJobsResponse, error)
	UpdateJobStatus(ctx context.Context, in *intapi.UpdateJobStatusRequest) (*intapi.UpdateJobStatusResponse, error)
}
