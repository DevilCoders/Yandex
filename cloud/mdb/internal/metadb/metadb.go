package metadb

import (
	"context"
	"io"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/ready"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/library/go/core/xerrors"
)

//go:generate ../../scripts/mockgen.sh MetaDB

// Storage errors
var (
	ErrDataNotFound    = xerrors.NewSentinel("data not found")
	ErrInvalidDataType = xerrors.NewSentinel("invalid data type in query response")
	ErrNoResultRecords = xerrors.NewSentinel("no result records")
)

type OnWorkerQueueEventsHandler func(context.Context, []WorkerQueueEvent) ([]int64, error)

// MetaDB is an interface to MetaDB
type MetaDB interface {
	io.Closer
	ready.Checker
	sqlutil.TxBinder

	// Operation returns operation with corresponding id
	Operation(ctx context.Context, id string) (*Operation, error)

	LastOperationsByType(ctx context.Context, taskType string, fromts time.Time) ([]CidAndTimestamp, error)

	FolderExtIDByClusterID(ctx context.Context, cid string, clusterType ClusterType) (string, error)

	ClusterInfo(ctx context.Context, cid string) (ClusterInfo, error)

	ShardByID(ctx context.Context, sid string) (ShardInfo, error)

	GetHostsBySubcid(ctx context.Context, subCid string) ([]Host, error)

	GetHostsByShardID(ctx context.Context, shardID string) ([]Host, error)

	ClusterShards(ctx context.Context, cid string) ([]ShardInfo, error)

	GetHostByFQDN(ctx context.Context, fqdn string) (Host, error)

	GetHostByVtypeID(ctx context.Context, vtypeID string) (Host, error)

	// ClustersRevs return all clusters revisions
	ClustersRevs(ctx context.Context) ([]ClusterRev, error)

	// ClusterHostsAtRev return all host from cluster at given revision
	ClusterHostsAtRev(ctx context.Context, cid string, rev int64) ([]Host, error)

	// ClusterHealthNonaggregatable return true if cluster marked as non aggregatable for mdb-health
	// pillar contains true at $.data.mdb_health.nonaggregatable path
	ClusterHealthNonaggregatable(ctx context.Context, cid string, rev int64) (bool, error)

	// ClusterAtRev return cluster at given rev.
	// Return ErrDataNotFound if no data found
	ClusterAtRev(ctx context.Context, cid string, rev int64) (Cluster, error)

	// get all clouds from metadb
	Clouds(ctx context.Context) ([]Cloud, error)

	// Set quota for the cloud
	SetCloudQuota(ctx context.Context, cloudID string, newResources ResourcesChange, xRequestID string) error

	// Create cloud
	CreateCloud(ctx context.Context, cloudID string, quota Resources, xRequestID string) error

	// OnUnsentWorkerQueueStartEvents call handler on worker_queue_events that has unset start_sent.
	// Lock events before call workers on it.
	// Mark as sent events that handler returns.
	OnUnsentWorkerQueueStartEvents(ctx context.Context, limit int64, handler OnWorkerQueueEventsHandler) (int, error)

	// OldestUnsetStartEvent return worker_queue_event that not marked as 'start-sent' and has oldest created_at
	OldestUnsetStartEvent(ctx context.Context) (WorkerQueueEvent, error)

	// OnUnsentWorkerQueueDoneEvents call handler on worker_queue_events that has unset finish_sent and was finished.
	// Lock events before call workers on it.
	// Mark as sent events that handler returns.
	OnUnsentWorkerQueueDoneEvents(ctx context.Context, limit int64, handler OnWorkerQueueEventsHandler) (int, error)

	// GetClusterCustomRolesAtRev get map of cluster hosts with custom roles, for example is ha/not ha for postgesql
	GetClusterCustomRolesAtRev(ctx context.Context, cType ClusterType, cid string, rev int64) (map[string]CustomRole, error)

	GetTaskTypeByAction(ctx context.Context, action string) ([]string, error)
}
