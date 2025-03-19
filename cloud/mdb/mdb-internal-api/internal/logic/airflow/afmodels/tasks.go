package afmodels

import "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/tasks"

const (
	TaskTypeClusterCreate         tasks.Type = "airflow_cluster_create"
	TaskTypeClusterDelete         tasks.Type = "airflow_cluster_delete"
	TaskTypeClusterDeleteMetadata tasks.Type = "airflow_cluster_delete_metadata"
)
