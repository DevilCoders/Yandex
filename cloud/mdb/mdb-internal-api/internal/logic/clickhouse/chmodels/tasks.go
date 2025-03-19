package chmodels

import "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/tasks"

const (
	TaskTypeClusterCreate         tasks.Type = "clickhouse_cluster_create"
	TaskTypeClusterRestore        tasks.Type = "clickhouse_cluster_restore"
	TaskTypeClusterModify         tasks.Type = "clickhouse_cluster_modify"
	TaskTypeClusterUpgrade        tasks.Type = "clickhouse_cluster_upgrade"
	TaskTypeMetadataUpdate        tasks.Type = "clickhouse_metadata_update"
	TaskTypeClusterDelete         tasks.Type = "clickhouse_cluster_delete"
	TaskTypeClusterDeleteMetadata tasks.Type = "clickhouse_cluster_delete_metadata"
	TaskTypeClusterPurge          tasks.Type = "clickhouse_cluster_purge"
	TaskTypeHostCreate            tasks.Type = "clickhouse_host_create"
	TaskTypeHostDelete            tasks.Type = "clickhouse_host_delete"
	TaskTypeHostUpdate            tasks.Type = "clickhouse_host_modify"
	TaskTypeZookeeperHostCreate   tasks.Type = "clickhouse_zookeeper_host_create"
	TaskTypeZookeeperHostDelete   tasks.Type = "clickhouse_zookeeper_host_delete"
	TaskTypeAddZookeeper          tasks.Type = "clickhouse_add_zookeeper"
	TaskTypeClusterStart          tasks.Type = "clickhouse_cluster_start"
	TaskTypeClusterStop           tasks.Type = "clickhouse_cluster_stop"
	TaskTypeClusterMove           tasks.Type = "clickhouse_cluster_move"
	TaskTypeClusterMoveNoOp       tasks.Type = "noop"
	TaskTypeClusterBackup         tasks.Type = "clickhouse_cluster_create_backup"

	TaskTypeClusterWaitBackupService tasks.Type = "clickhouse_cluster_wait_backup_service"

	TaskTypeDictionaryCreate tasks.Type = "clickhouse_dictionary_create"
	TaskTypeDictionaryDelete tasks.Type = "clickhouse_dictionary_delete"

	TaskTypeShardCreate tasks.Type = "clickhouse_shard_create"
	TaskTypeShardDelete tasks.Type = "clickhouse_shard_delete"

	TaskTypeMLModelCreate tasks.Type = "clickhouse_model_create"
	TaskTypeMLModelModify tasks.Type = "clickhouse_model_modify"
	TaskTypeMLModelDelete tasks.Type = "clickhouse_model_delete"

	TaskTypeFormatSchemaCreate tasks.Type = "clickhouse_format_schema_create"
	TaskTypeFormatSchemaModify tasks.Type = "clickhouse_format_schema_modify"
	TaskTypeFormatSchemaDelete tasks.Type = "clickhouse_format_schema_delete"

	TaskTypeUserCreate tasks.Type = "clickhouse_user_create"
	TaskTypeUserDelete tasks.Type = "clickhouse_user_delete"
	TaskTypeUserModify tasks.Type = "clickhouse_user_modify"

	TaskTypeDatabaseCreate tasks.Type = "clickhouse_database_create"
	TaskTypeDatabaseDelete tasks.Type = "clickhouse_database_delete"
)
