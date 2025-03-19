package mongomodels

import "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/tasks"

const (
	TaskTypeClusterCreate  tasks.Type = "mongodb_cluster_create"
	TaskTypeDatabaseCreate tasks.Type = "mongodb_database_create"
	TaskTypeUserCreate     tasks.Type = "mongodb_user_create"
	TaskTypeUserModify     tasks.Type = "mongodb_user_modify"
	TaskTypeUserDelete     tasks.Type = "mongodb_user_delete"
	TaskTypeResetupHosts   tasks.Type = "mongodb_cluster_resetup_hosts"
	TaskTypeRestartHosts   tasks.Type = "mongodb_cluster_restart_hosts"
	TaskTypeStepdownHosts  tasks.Type = "mongodb_cluster_stepdown_hosts"
	TaskTypeDeleteBackup   tasks.Type = "mongodb_backup_delete"
)
