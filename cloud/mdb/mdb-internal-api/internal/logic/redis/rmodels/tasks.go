package rmodels

import "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/tasks"

const (
	TaskTypeFailover  tasks.Type = "redis_cluster_start_failover"
	TaskTypeRebalance tasks.Type = "redis_cluster_rebalance"

	TaskTypeClusterStart    tasks.Type = "redis_cluster_start"
	TaskTypeClusterStop     tasks.Type = "redis_cluster_stop"
	TaskTypeClusterMove     tasks.Type = "redis_cluster_move"
	TaskTypeClusterMoveNoOp tasks.Type = "noop"

	TaskTypeClusterBackup tasks.Type = "redis_cluster_create_backup"

	TaskTypeHostCreate      tasks.Type = "redis_host_create"
	TaskTypeShardHostCreate tasks.Type = "redis_shard_host_create"
)
