package chmodels

import (
	"strings"
	"time"

	"github.com/golang/protobuf/proto"
	"google.golang.org/protobuf/types/known/timestamppb"

	chv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/clickhouse/v1"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/backups"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
)

const (
	OperationTypeClusterCreate  operations.Type = "clickhouse_cluster_create"
	OperationTypeClusterRestore operations.Type = "clickhouse_cluster_restore"
	OperationTypeClusterModify  operations.Type = "clickhouse_cluster_modify"
	OperationTypeMetadataUpdate operations.Type = "clickhouse_metadata_update"
	OperationTypeClusterDelete  operations.Type = "clickhouse_cluster_delete"
	OperationTypeAddZookeeper   operations.Type = "clickhouse_add_zookeeper"
	OperationTypeClusterStart   operations.Type = "clickhouse_cluster_start"
	OperationTypeClusterStop    operations.Type = "clickhouse_cluster_stop"
	OperationTypeClusterMove    operations.Type = "clickhouse_cluster_move"
	OperationTypeClusterBackup  operations.Type = "clickhouse_cluster_create_backup"
	OperationTypeOnlineResetup  operations.Type = "clickhouse_cluster_online_resetup"

	OperationTypeHostCreate operations.Type = "clickhouse_host_create"
	OperationTypeHostUpdate operations.Type = "clickhouse_host_modify"
	OperationTypeHostDelete operations.Type = "clickhouse_host_delete"

	OperationTypeShardCreate operations.Type = "clickhouse_shard_create"
	OperationTypeShardDelete operations.Type = "clickhouse_shard_delete"
	OperationTypeShardModify operations.Type = "clickhouse_shard_modify"

	OperationTypeDictionaryCreate operations.Type = "clickhouse_add_dictionary"
	OperationTypeDictionaryUpdate operations.Type = "clickhouse_update_dictionary"
	OperationTypeDictionaryDelete operations.Type = "clickhouse_delete_dictionary"

	OperationTypeUserCreate           operations.Type = "clickhouse_user_create"
	OperationTypeUserModify           operations.Type = "clickhouse_user_modify"
	OperationTypeUserDelete           operations.Type = "clickhouse_user_delete"
	OperationTypeUserGrantPermission  operations.Type = "clickhouse_user_grant_permission"
	OperationTypeUserRevokePermission operations.Type = "clickhouse_user_revoke_permission"

	OperationTypeDatabaseAdd    operations.Type = "clickhouse_database_add"
	OperationTypeDatabaseDelete operations.Type = "clickhouse_database_delete"

	OperationTypeMLModelCreate operations.Type = "clickhouse_model_create"
	OperationTypeMLModelModify operations.Type = "clickhouse_model_modify"
	OperationTypeMLModelDelete operations.Type = "clickhouse_model_delete"

	OperationTypeFormatSchemaCreate operations.Type = "clickhouse_format_schema_create"
	OperationTypeFormatSchemaModify operations.Type = "clickhouse_format_schema_modify"
	OperationTypeFormatSchemaDelete operations.Type = "clickhouse_format_schema_delete"

	OperationTypeShardGroupCreate operations.Type = "clickhouse_shard_group_create"
	OperationTypeShardGroupUpdate operations.Type = "clickhouse_shard_group_update"
	OperationTypeShardGroupDelete operations.Type = "clickhouse_shard_group_delete"

	OperationTypeMaintenanceReschedule operations.Type = "clickhouse_maintenance_reschedule"
)

func init() {
	operations.Register(
		OperationTypeClusterCreate,
		"Create ClickHouse cluster",
		MetadataCreateCluster{},
	)
	operations.Register(
		OperationTypeClusterRestore,
		"Create new ClickHouse cluster from the backup",
		MetadataRestoreCluster{},
	)
	operations.Register(
		OperationTypeClusterModify,
		"Modify ClickHouse cluster",
		MetadataModifyCluster{},
	)
	operations.Register(
		OperationTypeMetadataUpdate,
		"Update ClickHouse cluster metadata",
		MetadataModifyCluster{},
	)
	operations.Register(
		OperationTypeClusterDelete,
		"Delete ClickHouse cluster",
		MetadataDeleteCluster{},
	)
	operations.Register(
		OperationTypeClusterStart,
		"Start ClickHouse cluster",
		MetadataStartCluster{},
	)
	operations.Register(
		OperationTypeClusterStop,
		"Stop ClickHouse cluster",
		MetadataStopCluster{},
	)
	operations.Register(
		OperationTypeClusterMove,
		"Move ClickHouse cluster",
		MetadataMoveCluster{},
	)
	operations.Register(
		OperationTypeClusterBackup,
		"Create a backup for ClickHouse cluster",
		MetadataBackupCluster{},
	)
	operations.Register(
		OperationTypeAddZookeeper,
		"Add ZooKeeper to ClickHouse cluster",
		MetadataAddZookeeper{},
	)
	operations.Register(
		OperationTypeHostCreate,
		"Add hosts to ClickHouse cluster",
		MetadataCreateHosts{},
	)
	operations.Register(
		OperationTypeHostUpdate,
		"Update hosts in ClickHouse cluster",
		MetadataUpdateHosts{},
	)
	operations.Register(
		OperationTypeHostDelete,
		"Delete hosts from ClickHouse cluster",
		MetadataDeleteHosts{},
	)
	operations.Register(
		OperationTypeShardCreate,
		"Add shard to ClickHouse cluster",
		MetadataCreateShard{},
	)
	operations.Register(
		OperationTypeShardDelete,
		"Delete shard from ClickHouse cluster",
		MetadataDeleteShard{},
	)
	operations.Register(
		OperationTypeShardModify,
		"Modify shard in ClickHouse cluster",
		MetadataModifyShard{},
	)
	operations.Register(
		OperationTypeUserCreate,
		"Create user in ClickHouse cluster",
		MetadataCreateUser{},
	)
	operations.Register(
		OperationTypeUserModify,
		"Update user in ClickHouse cluster",
		MetadataModifyUser{},
	)
	operations.Register(
		OperationTypeUserDelete,
		"Delete user in ClickHouse cluster",
		MetadataDeleteUser{},
	)
	operations.Register(
		OperationTypeUserGrantPermission,
		"Grant permission to user in ClickHouse cluster",
		MetadataGrantUserPermission{},
	)
	operations.Register(
		OperationTypeUserRevokePermission,
		"Revoke permission from user in ClickHouse cluster",
		MetadataRevokeUserPermission{},
	)
	operations.Register(
		OperationTypeDatabaseAdd,
		"Add database to ClickHouse cluster",
		MetadataCreateDatabase{},
	)
	operations.Register(
		OperationTypeDatabaseDelete,
		"Delete database from ClickHouse cluster",
		MetadataDeleteDatabase{},
	)
	operations.Register(
		OperationTypeMLModelCreate,
		"Add ML model in ClickHouse cluster",
		MetadataCreateMLModel{},
	)
	operations.Register(
		OperationTypeMLModelModify,
		"Modify ML model in ClickHouse cluster",
		MetadataUpdateMLModel{},
	)
	operations.Register(
		OperationTypeMLModelDelete,
		"Delete ML model in ClickHouse cluster",
		MetadataDeleteMLModel{},
	)
	operations.Register(
		OperationTypeFormatSchemaCreate,
		"Add format schema in ClickHouse cluster",
		MetadataCreateFormatSchema{},
	)
	operations.Register(
		OperationTypeFormatSchemaModify,
		"Modify format schema in ClickHouse cluster",
		MetadataUpdateFormatSchema{},
	)
	operations.Register(
		OperationTypeFormatSchemaDelete,
		"Delete format schema in ClickHouse cluster",
		MetadataDeleteFormatSchema{},
	)
	operations.Register(
		OperationTypeShardGroupCreate,
		"Add shard group in ClickHouse cluster",
		MetadataCreateClusterShardGroup{},
	)
	operations.Register(
		OperationTypeShardGroupUpdate,
		"Update shard group in ClickHouse cluster",
		MetadataUpdateClusterShardGroup{},
	)
	operations.Register(
		OperationTypeShardGroupDelete,
		"Delete shard group in ClickHouse cluster",
		MetadataDeleteClusterShardGroup{},
	)
	operations.Register(
		OperationTypeDictionaryCreate,
		"Add external dictionary in ClickHouse cluster",
		MetadataCreateDictionary{},
	)
	operations.Register(
		OperationTypeDictionaryUpdate,
		"Update external dictionary in ClickHouse cluster",
		MetadataUpdateDictionary{},
	)
	operations.Register(
		OperationTypeDictionaryDelete,
		"Delete external dictionary in ClickHouse cluster",
		MetadataDeleteDictionary{},
	)

	operations.Register(
		OperationTypeMaintenanceReschedule,
		"Reschedule maintenance in ClickHouse cluster",
		MetadataRescheduleMaintenance{},
	)

	operations.Register(
		OperationTypeOnlineResetup,
		"Resetup host in ClickHouse cluster",
		MetadataResetupCluster{},
	)
}

type MetadataCreateCluster struct{}

func (md MetadataCreateCluster) Build(op operations.Operation) proto.Message {
	return &chv1.CreateClusterMetadata{
		ClusterId: op.TargetID,
	}
}

type MetadataRestoreCluster struct {
	SourceClusterID string `json:"source_cid"`
	BackupID        string `json:"backup_id"`
}

func (md MetadataRestoreCluster) Build(op operations.Operation) proto.Message {
	backupIDs := strings.Split(md.BackupID, DefaultBackupIDSeparator)

	var globalBackupIDs []string
	for _, backupID := range backupIDs {
		globalBackupIDs = append(globalBackupIDs, backups.EncodeGlobalBackupID(md.SourceClusterID, backupID))
	}

	return &chv1.RestoreClusterMetadata{
		ClusterId: op.TargetID,
		BackupId:  strings.Join(globalBackupIDs, DefaultBackupIDSeparator),
	}
}

type MetadataModifyCluster struct{}

func (md MetadataModifyCluster) Build(op operations.Operation) proto.Message {
	return &chv1.UpdateClusterMetadata{
		ClusterId: op.TargetID,
	}
}

type MetadataDeleteCluster struct{}

func (md MetadataDeleteCluster) Build(op operations.Operation) proto.Message {
	return &chv1.DeleteClusterMetadata{
		ClusterId: op.TargetID,
	}
}

type MetadataAddZookeeper struct{}

func (md MetadataAddZookeeper) Build(op operations.Operation) proto.Message {
	return &chv1.AddClusterZookeeperMetadata{
		ClusterId: op.TargetID,
	}
}

type MetadataStartCluster struct{}

func (md MetadataStartCluster) Build(op operations.Operation) proto.Message {
	return &chv1.StartClusterMetadata{
		ClusterId: op.TargetID,
	}
}

type MetadataStopCluster struct{}

func (md MetadataStopCluster) Build(op operations.Operation) proto.Message {
	return &chv1.StopClusterMetadata{
		ClusterId: op.TargetID,
	}
}

type MetadataMoveCluster struct {
	SourceFolderID      string `json:"source_folder_id"`
	DestinationFolderID string `json:"destination_folder_id"`
}

func (md MetadataMoveCluster) Build(op operations.Operation) proto.Message {
	return &chv1.MoveClusterMetadata{
		ClusterId:           op.TargetID,
		SourceFolderId:      md.SourceFolderID,
		DestinationFolderId: md.DestinationFolderID,
	}
}

type MetadataBackupCluster struct{}

func (md MetadataBackupCluster) Build(op operations.Operation) proto.Message {
	return &chv1.BackupClusterMetadata{
		ClusterId: op.TargetID,
	}
}

type MetadataResetupCluster struct{}

func (md MetadataResetupCluster) Build(op operations.Operation) proto.Message {
	return &chv1.UpdateClusterMetadata{
		ClusterId: op.TargetID,
	}
}

type MetadataCreateHosts struct {
	HostNames []string `json:"host_names"`
}

func (md MetadataCreateHosts) Build(op operations.Operation) proto.Message {
	return &chv1.AddClusterHostsMetadata{
		ClusterId: op.TargetID,
		HostNames: md.HostNames,
	}
}

type MetadataUpdateHosts struct {
	HostNames []string `json:"host_names"`
}

func (md MetadataUpdateHosts) Build(op operations.Operation) proto.Message {
	return &chv1.UpdateClusterHostsMetadata{
		ClusterId: op.TargetID,
		HostNames: md.HostNames,
	}
}

type MetadataCreateShard struct {
	ShardName string `json:"shard_name"`
}

func (md MetadataCreateShard) Build(op operations.Operation) proto.Message {
	return &chv1.AddClusterShardMetadata{
		ClusterId: op.TargetID,
		ShardName: md.ShardName,
	}
}

type MetadataDeleteShard struct {
	ShardName string `json:"shard_name"`
}

func (md MetadataDeleteShard) Build(op operations.Operation) proto.Message {
	return &chv1.DeleteClusterShardMetadata{
		ClusterId: op.TargetID,
		ShardName: md.ShardName,
	}
}

type MetadataModifyShard struct {
	ShardName string `json:"shard_name"`
}

func (md MetadataModifyShard) Build(op operations.Operation) proto.Message {
	return &chv1.UpdateClusterShardMetadata{
		ClusterId: op.TargetID,
		ShardName: md.ShardName,
	}
}

type MetadataDeleteHosts struct {
	HostNames []string `json:"host_names"`
}

func (md MetadataDeleteHosts) Build(op operations.Operation) proto.Message {
	return &chv1.DeleteClusterHostsMetadata{
		ClusterId: op.TargetID,
		HostNames: md.HostNames,
	}
}

type MetadataCreateDatabase struct {
	DatabaseName string `json:"database_name"`
}

func (md MetadataCreateDatabase) Build(op operations.Operation) proto.Message {
	return &chv1.CreateDatabaseMetadata{
		ClusterId:    op.TargetID,
		DatabaseName: md.DatabaseName,
	}
}

type MetadataDeleteDatabase struct {
	DatabaseName string `json:"database_name"`
}

func (md MetadataDeleteDatabase) Build(op operations.Operation) proto.Message {
	return &chv1.DeleteDatabaseMetadata{
		ClusterId:    op.TargetID,
		DatabaseName: md.DatabaseName,
	}
}

type MetadataCreateUser struct {
	UserName string `json:"user_name"`
}

func (md MetadataCreateUser) Build(op operations.Operation) proto.Message {
	return &chv1.CreateUserMetadata{
		ClusterId: op.TargetID,
		UserName:  md.UserName,
	}
}

type MetadataModifyUser struct {
	UserName string `json:"user_name"`
}

func (md MetadataModifyUser) Build(op operations.Operation) proto.Message {
	return &chv1.UpdateUserMetadata{
		ClusterId: op.TargetID,
		UserName:  md.UserName,
	}
}

type MetadataDeleteUser struct {
	UserName string `json:"user_name"`
}

func (md MetadataDeleteUser) Build(op operations.Operation) proto.Message {
	return &chv1.DeleteUserMetadata{
		ClusterId: op.TargetID,
		UserName:  md.UserName,
	}
}

type MetadataGrantUserPermission struct {
	UserName string `json:"user_name"`
}

func (md MetadataGrantUserPermission) Build(op operations.Operation) proto.Message {
	return &chv1.GrantUserPermissionMetadata{
		ClusterId: op.TargetID,
		UserName:  md.UserName,
	}
}

type MetadataRevokeUserPermission struct {
	UserName string `json:"user_name"`
}

func (md MetadataRevokeUserPermission) Build(op operations.Operation) proto.Message {
	return &chv1.RevokeUserPermissionMetadata{
		ClusterId: op.TargetID,
		UserName:  md.UserName,
	}
}

type MetadataCreateMLModel struct {
	MLModelName string `json:"ml_model_name"`
}

func (md MetadataCreateMLModel) Build(op operations.Operation) proto.Message {
	return &chv1.CreateMlModelMetadata{
		ClusterId:   op.TargetID,
		MlModelName: md.MLModelName,
	}
}

type MetadataUpdateMLModel struct {
	MLModelName string `json:"ml_model_name"`
}

func (md MetadataUpdateMLModel) Build(op operations.Operation) proto.Message {
	return &chv1.UpdateMlModelRequest{
		ClusterId:   op.TargetID,
		MlModelName: md.MLModelName,
	}
}

type MetadataDeleteMLModel struct {
	MLModelName string `json:"ml_model_name"`
}

func (md MetadataDeleteMLModel) Build(op operations.Operation) proto.Message {
	return &chv1.DeleteMlModelMetadata{
		ClusterId:   op.TargetID,
		MlModelName: md.MLModelName,
	}
}

type MetadataCreateFormatSchema struct {
	FormatSchemaName string `json:"format_schema_name"`
}

func (md MetadataCreateFormatSchema) Build(op operations.Operation) proto.Message {
	return &chv1.CreateFormatSchemaMetadata{
		ClusterId:        op.TargetID,
		FormatSchemaName: md.FormatSchemaName,
	}
}

type MetadataUpdateFormatSchema struct {
	FormatSchemaName string `json:"format_schema_name"`
}

func (md MetadataUpdateFormatSchema) Build(op operations.Operation) proto.Message {
	return &chv1.UpdateFormatSchemaMetadata{
		ClusterId:        op.TargetID,
		FormatSchemaName: md.FormatSchemaName,
	}
}

type MetadataDeleteFormatSchema struct {
	FormatSchemaName string `json:"format_schema_name"`
}

func (md MetadataDeleteFormatSchema) Build(op operations.Operation) proto.Message {
	return &chv1.DeleteFormatSchemaMetadata{
		ClusterId:        op.TargetID,
		FormatSchemaName: md.FormatSchemaName,
	}
}

type MetadataCreateClusterShardGroup struct {
	ShardGroupName string `json:"shard_group_name"`
}

func (md MetadataCreateClusterShardGroup) Build(op operations.Operation) proto.Message {
	return &chv1.CreateClusterShardGroupMetadata{
		ClusterId:      op.TargetID,
		ShardGroupName: md.ShardGroupName,
	}
}

type MetadataUpdateClusterShardGroup struct {
	ShardGroupName string `json:"shard_group_name"`
}

func (md MetadataUpdateClusterShardGroup) Build(op operations.Operation) proto.Message {
	return &chv1.UpdateClusterShardGroupMetadata{
		ClusterId:      op.TargetID,
		ShardGroupName: md.ShardGroupName,
	}
}

type MetadataDeleteClusterShardGroup struct {
	ShardGroupName string `json:"shard_group_name"`
}

func (md MetadataDeleteClusterShardGroup) Build(op operations.Operation) proto.Message {
	return &chv1.DeleteClusterShardGroupMetadata{
		ClusterId:      op.TargetID,
		ShardGroupName: md.ShardGroupName,
	}
}

type MetadataCreateDictionary struct{}

func (md MetadataCreateDictionary) Build(op operations.Operation) proto.Message {
	return &chv1.CreateClusterExternalDictionaryMetadata{
		ClusterId: op.TargetID,
	}
}

type MetadataUpdateDictionary struct{}

func (md MetadataUpdateDictionary) Build(op operations.Operation) proto.Message {
	return &chv1.UpdateClusterExternalDictionaryMetadata{
		ClusterId: op.TargetID,
	}
}

type MetadataDeleteDictionary struct{}

func (md MetadataDeleteDictionary) Build(op operations.Operation) proto.Message {
	return &chv1.DeleteClusterExternalDictionaryMetadata{
		ClusterId: op.TargetID,
	}
}

type MetadataRescheduleMaintenance struct {
	DelayedUntil time.Time `json:"delayed_until"`
}

func (md MetadataRescheduleMaintenance) Build(op operations.Operation) proto.Message {
	return &chv1.RescheduleMaintenanceMetadata{
		ClusterId:    op.TargetID,
		DelayedUntil: timestamppb.New(md.DelayedUntil),
	}
}
