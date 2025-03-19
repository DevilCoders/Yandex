package ssmodels

import "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/tasks"

const (
	TaskTypeClusterCreate         tasks.Type = "sqlserver_cluster_create"
	TaskTypeClusterDelete         tasks.Type = "sqlserver_cluster_delete"
	TaskTypeClusterDeleteMetadata tasks.Type = "sqlserver_cluster_delete_metadata"
	TaskTypeClusterPurge          tasks.Type = "sqlserver_cluster_purge"
	TaskTypeClusterModify         tasks.Type = "sqlserver_cluster_modify"
	TaskTypeMetadataUpdate        tasks.Type = "sqlserver_metadata_update"
	TaskTypeClusterBackup         tasks.Type = "sqlserver_cluster_create_backup"
	TaskTypeClusterRestore        tasks.Type = "sqlserver_cluster_restore"
	TaskTypeDatabaseCreate        tasks.Type = "sqlserver_database_create"
	TaskTypeDatabaseDelete        tasks.Type = "sqlserver_database_delete"
	TaskTypeDatabaseRestore       tasks.Type = "sqlserver_database_restore"
	TaskTypeHostUpdate            tasks.Type = "sqlserver_host_modify"
	TaskTypeUserCreate            tasks.Type = "sqlserver_user_create"
	TaskTypeUserModify            tasks.Type = "sqlserver_user_modify"
	TaskTypeUserDelete            tasks.Type = "sqlserver_user_delete"
	TaskTypeClusterStart          tasks.Type = "sqlserver_cluster_start"
	TaskTypeClusterStop           tasks.Type = "sqlserver_cluster_stop"
	TaskTypeClusterStartFailover  tasks.Type = "sqlserver_cluster_start_failover"
	TaskTypeDatabaseBackupImport  tasks.Type = "sqlserver_database_backup_import"
	TaskTypeDatabaseBackupExport  tasks.Type = "sqlserver_database_backup_export"
)
