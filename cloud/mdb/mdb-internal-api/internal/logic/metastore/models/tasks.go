package models

import "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/tasks"

const (
	TaskTypeClusterCreate         tasks.Type = "metastore_cluster_create"
	TaskTypeClusterDelete         tasks.Type = "metastore_cluster_delete"
	TaskTypeClusterDeleteMetadata tasks.Type = "metastore_cluster_delete_metadata"
)
