package gpmodels

import "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/tasks"

const (
	TaskTypeClusterCreate         tasks.Type = "greenplum_cluster_create"
	TaskTypeClusterRestore        tasks.Type = "greenplum_cluster_restore"
	TaskTypeClusterDelete         tasks.Type = "greenplum_cluster_delete"
	TaskTypeClusterDeleteMetadata tasks.Type = "greenplum_cluster_delete_metadata"
	TaskTypeClusterPurge          tasks.Type = "greenplum_cluster_purge"

	TaskTypeClusterStart tasks.Type = "greenplum_cluster_start"
	TaskTypeClusterStop  tasks.Type = "greenplum_cluster_stop"

	TaskTypeClusterModify  tasks.Type = "greenplum_cluster_modify"
	TaskTypeMetadataUpdate tasks.Type = "greenplum_metadata_update"

	TaskTypeHostCreate tasks.Type = "greenplum_host_create"
)
