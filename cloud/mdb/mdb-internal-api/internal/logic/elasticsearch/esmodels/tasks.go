package esmodels

import "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/tasks"

const (
	TaskTypeClusterCreate         tasks.Type = "elasticsearch_cluster_create"
	TaskTypeClusterModify         tasks.Type = "elasticsearch_cluster_modify"
	TaskTypeMetadataUpdate        tasks.Type = "elasticsearch_metadata_update"
	TaskTypeClusterUpgrade        tasks.Type = "elasticsearch_cluster_upgrade"
	TaskTypeClusterDelete         tasks.Type = "elasticsearch_cluster_delete"
	TaskTypeClusterStart          tasks.Type = "elasticsearch_cluster_start"
	TaskTypeClusterStop           tasks.Type = "elasticsearch_cluster_stop"
	TaskTypeClusterDeleteMetadata tasks.Type = "elasticsearch_cluster_delete_metadata"
	TaskTypeClusterPurge          tasks.Type = "elasticsearch_cluster_purge"
	TaskTypeUserCreate            tasks.Type = "elasticsearch_user_create"
	TaskTypeUserModify            tasks.Type = "elasticsearch_user_modify"
	TaskTypeUserDelete            tasks.Type = "elasticsearch_user_delete"
	TaskTypeHostCreate            tasks.Type = "elasticsearch_host_create"
	TaskTypeHostDelete            tasks.Type = "elasticsearch_host_delete"
	TaskTypeClusterBackup         tasks.Type = "elasticsearch_cluster_create_backup"
	TaskTypeClusterRestore        tasks.Type = "elasticsearch_cluster_restore"
)
