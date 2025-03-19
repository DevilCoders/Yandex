package metadb

import (
	"context"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/ready"
	"a.yandex-team.ru/cloud/mdb/mdb-maintenance/internal/models"
)

//go:generate ../../../scripts/mockgen.sh MetaDB

// MetaDB API
type MetaDB interface {
	ready.Checker

	Begin(ctx context.Context) (context.Context, error)
	Commit(ctx context.Context) error
	Rollback(ctx context.Context) error
	SelectClusters(ctx context.Context, cfg models.MaintenanceTaskConfig) ([]models.Cluster, error)
	ChangePillar(ctx context.Context, cid string, cfg models.MaintenanceTaskConfig) error
	SelectTaskArgs(ctx context.Context, cid string, cfg models.MaintenanceTaskConfig) (map[string]interface{}, error)
	LockCluster(ctx context.Context, cid string) (int64, error)
	LockFutureCluster(ctx context.Context, cid string) (models.ClusterRevs, error)
	CompleteFutureClusterChange(ctx context.Context, cid string, rev, nextRev int64) error
	CompleteClusterChange(ctx context.Context, cid string, rev int64) error
	MaintenanceTasks(ctx context.Context, configID string) ([]models.MaintenanceTask, error)
	MaintenanceTasksByCIDs(ctx context.Context, CIDs []string) ([]models.MaintenanceTask, error)
	PlanMaintenanceTask(ctx context.Context, task models.PlanMaintenanceTaskRequest) error
	CompleteMaintenanceTask(ctx context.Context, cid, configID string) error
	GetClusterByInstanceID(ctx context.Context, instanceID string, clusterType string) (models.Cluster, error)
	SelectTimeout(ctx context.Context, cid string, cfg models.MaintenanceTaskConfig) (time.Duration, error)
}
