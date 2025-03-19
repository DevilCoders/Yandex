package osmodels

import "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/tasks"

const (
	TaskTypeClusterCreate         tasks.Type = "opensearch_cluster_create"
	TaskTypeClusterModify         tasks.Type = "opensearch_cluster_modify"
	TaskTypeMetadataUpdate        tasks.Type = "opensearch_metadata_update"
	TaskTypeClusterUpgrade        tasks.Type = "opensearch_cluster_upgrade"
	TaskTypeClusterDelete         tasks.Type = "opensearch_cluster_delete"
	TaskTypeClusterStart          tasks.Type = "opensearch_cluster_start"
	TaskTypeClusterStop           tasks.Type = "opensearch_cluster_stop"
	TaskTypeClusterDeleteMetadata tasks.Type = "opensearch_cluster_delete_metadata"
	TaskTypeClusterPurge          tasks.Type = "opensearch_cluster_purge"
	TaskTypeUserCreate            tasks.Type = "opensearch_user_create"
	TaskTypeUserModify            tasks.Type = "opensearch_user_modify"
	TaskTypeUserDelete            tasks.Type = "opensearch_user_delete"
	TaskTypeHostCreate            tasks.Type = "opensearch_host_create"
	TaskTypeHostDelete            tasks.Type = "opensearch_host_delete"
	TaskTypeClusterBackup         tasks.Type = "opensearch_cluster_create_backup"
	TaskTypeClusterRestore        tasks.Type = "opensearch_cluster_restore"
)
