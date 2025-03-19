package ssmodels

import (
	"github.com/golang/protobuf/proto"

	ssv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/sqlserver/v1"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/backups"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
)

const (
	OperationTypeClusterCreate        operations.Type = "sqlserver_cluster_create"
	OperationTypeClusterModify        operations.Type = "sqlserver_cluster_modify"
	OperationTypeMetadataUpdate       operations.Type = "sqlserver_metadata_update"
	OperationTypeClusterDelete        operations.Type = "sqlserver_cluster_delete"
	OperationTypeClusterBackup        operations.Type = "sqlserver_cluster_create_backup"
	OperationTypeClusterRestore       operations.Type = "sqlserver_cluster_restore"
	OperationTypeClusterStart         operations.Type = "sqlserver_cluster_start"
	OperationTypeClusterStop          operations.Type = "sqlserver_cluster_stop"
	OperationTypeClusterStartFailover operations.Type = "sqlserver_cluster_start_failover"
	OperationTypeUserCreate           operations.Type = "sqlserver_user_create"
	OperationTypeUserModify           operations.Type = "sqlserver_user_modify"
	OperationTypeUserDelete           operations.Type = "sqlserver_user_delete"
	OperationTypeUserPermissionGrant  operations.Type = "sqlserver_permission_grant"
	OperationTypeUserPermissionRevoke operations.Type = "sqlserver_permission_revoke"
	OperationTypeDatabaseAdd          operations.Type = "sqlserver_database_add"
	OperationTypeDatabaseDelete       operations.Type = "sqlserver_database_delete"
	OperationTypeDatabaseRestore      operations.Type = "sqlserver_database_restore"
	OperationTypeDatabaseImport       operations.Type = "sqlserver_database_import"
	OperationTypeDatabaseExport       operations.Type = "sqlserver_database_export"
	OperationTypeHostUpdate           operations.Type = "sqlserver_host_modify"
)

func init() {
	operations.Register(
		OperationTypeClusterCreate,
		"Create Microsoft SQLServer cluster",
		MetadataCreateCluster{},
	)

	operations.Register(
		OperationTypeClusterModify,
		"Modify Microsoft SQLServer cluster",
		MetadataModifyCluster{},
	)

	operations.Register(
		OperationTypeMetadataUpdate,
		"Update Microsoft SQLServer cluster metadata",
		MetadataModifyCluster{},
	)

	operations.Register(
		OperationTypeClusterDelete,
		"Delete Microsoft SQLServer cluster",
		MetadataDeleteCluster{},
	)

	operations.Register(
		OperationTypeClusterBackup,
		"Create a backup for Microsoft SQLServer cluster",
		MetadataBackupCluster{},
	)

	operations.Register(
		OperationTypeClusterRestore,
		"Create new Microsoft SQLServer cluster from backup",
		MetadataRestoreCluster{},
	)

	operations.Register(
		OperationTypeClusterStart,
		"Start Microsoft SQLServer cluster",
		MetadataStartCluster{},
	)

	operations.Register(
		OperationTypeClusterStop,
		"Stop Microsoft SQLServer cluster",
		MetadataStopCluster{},
	)

	operations.Register(
		OperationTypeClusterStartFailover,
		"Start Microsoft SQLServer cluster failover",
		MetadataStartClusterFailover{},
	)

	operations.Register(
		OperationTypeUserCreate,
		"Create user in Microsoft SQLServer cluster",
		MetadataCreateUser{},
	)

	operations.Register(
		OperationTypeUserModify,
		"Modify user in Microsoft SQLServer cluster",
		MetadataUpdateUser{},
	)

	operations.Register(
		OperationTypeUserDelete,
		"Delete user from Microsoft SQLServer cluster",
		MetadataDeleteUser{},
	)

	operations.Register(
		OperationTypeDatabaseAdd,
		"Add database to Microsoft SQLServer cluster",
		MetadataCreateDatabase{},
	)

	operations.Register(
		OperationTypeDatabaseRestore,
		"Restore database to Microsoft SQLServer cluster",
		MetadataRestoreDatabase{},
	)

	operations.Register(
		OperationTypeDatabaseImport,
		"Import database from external storage to Microsoft SQLServer cluster",
		MetadataImportDatabaseBackup{},
	)

	operations.Register(
		OperationTypeDatabaseExport,
		"Export database from Microsoft SQLServer cluster to external storage",
		MetadataExportDatabaseBackup{},
	)

	operations.Register(
		OperationTypeDatabaseDelete,
		"Delete database from Microsoft SQLServer cluster",
		MetadataDeleteDatabase{},
	)

	operations.Register(
		OperationTypeUserPermissionGrant,
		"Grant user permissions in Microsoft SQLServer cluster",
		MetadataGrantPermission{},
	)

	operations.Register(
		OperationTypeUserPermissionRevoke,
		"Revoke user permissions in Microsoft SQLServer cluster",
		MetadataRevokePermission{},
	)

	operations.Register(
		OperationTypeHostUpdate,
		"Update SQL Server cluster host",
		MetadataUpdateHosts{},
	)
}

type MetadataCreateCluster struct{}

func (md MetadataCreateCluster) Build(op operations.Operation) proto.Message {
	return &ssv1.CreateClusterMetadata{
		ClusterId: op.TargetID,
	}
}

type MetadataModifyCluster struct{}

func (md MetadataModifyCluster) Build(op operations.Operation) proto.Message {
	return &ssv1.UpdateClusterMetadata{
		ClusterId: op.TargetID,
	}
}

type MetadataDeleteCluster struct{}

func (md MetadataDeleteCluster) Build(op operations.Operation) proto.Message {
	return &ssv1.DeleteClusterMetadata{
		ClusterId: op.TargetID,
	}
}

type MetadataBackupCluster struct{}

func (md MetadataBackupCluster) Build(op operations.Operation) proto.Message {
	return &ssv1.BackupClusterMetadata{
		ClusterId: op.TargetID,
	}
}

type MetadataRestoreCluster struct {
	BackupID        string `json:"backup_id"`
	SourceClusterID string `json:"source_cid"`
}

func (md MetadataRestoreCluster) Build(op operations.Operation) proto.Message {
	return &ssv1.RestoreClusterMetadata{
		ClusterId: op.TargetID,
		BackupId:  backups.EncodeGlobalBackupID(md.SourceClusterID, md.BackupID),
	}
}

type MetadataStartCluster struct{}

func (md MetadataStartCluster) Build(op operations.Operation) proto.Message {
	return &ssv1.StartClusterMetadata{
		ClusterId: op.TargetID,
	}
}

type MetadataStopCluster struct{}

func (md MetadataStopCluster) Build(op operations.Operation) proto.Message {
	return &ssv1.StopClusterMetadata{
		ClusterId: op.TargetID,
	}
}

type MetadataStartClusterFailover struct{}

func (md MetadataStartClusterFailover) Build(op operations.Operation) proto.Message {
	return &ssv1.StartClusterFailoverMetadata{
		ClusterId: op.TargetID,
	}
}

type DatabaseMetadata interface {
	GetDatabaseName() string
}
type RestoreDatabaseMetadata interface {
	GetClusterID() string
	GetDatabaseName() string
	GetFromDatabase() string
	GetBackupID() string
}

type MetadataCreateDatabase struct {
	DatabaseName string `json:"database_name"`
}

type MetadataRestoreDatabase struct {
	ClusterID    string `json:"cluster_id"`
	DatabaseName string `json:"database_name"`
	FromDatabase string `json:"from_database"`
	BackupID     string `json:"backup_id"`
}

func (md MetadataRestoreDatabase) Build(op operations.Operation) proto.Message {
	return &ssv1.RestoreDatabaseMetadata{
		ClusterId:    op.TargetID,
		DatabaseName: md.DatabaseName,
		FromDatabase: md.FromDatabase,
		BackupId:     md.BackupID,
	}
}

func (md MetadataRestoreDatabase) GetClusterID() string {
	return md.ClusterID
}

func (md MetadataRestoreDatabase) GetDatabaseName() string {
	return md.DatabaseName
}

func (md MetadataRestoreDatabase) GetFromDatabase() string {
	return md.FromDatabase
}

func (md MetadataRestoreDatabase) GetBackupID() string {
	return md.BackupID
}

func (md MetadataCreateDatabase) Build(op operations.Operation) proto.Message {
	return &ssv1.CreateDatabaseMetadata{
		ClusterId:    op.TargetID,
		DatabaseName: md.DatabaseName,
	}
}

func (md MetadataCreateDatabase) GetDatabaseName() string {
	return md.DatabaseName
}

type MetadataDeleteDatabase struct {
	DatabaseName string `json:"database_name"`
}

func (md MetadataDeleteDatabase) Build(op operations.Operation) proto.Message {
	return &ssv1.DeleteDatabaseMetadata{
		ClusterId:    op.TargetID,
		DatabaseName: md.DatabaseName,
	}
}

type ImportDatabaseBackupMetadata interface {
	GetClusterID() string
	GetDatabaseName() string
	GetS3Bucket() string
	GetS3Path() string
}

type MetadataImportDatabaseBackup struct {
	ClusterID    string `json:"cluster_id"`
	DatabaseName string `json:"database_name"`
	S3Bucket     string `json:"s3_bucket"`
	S3Path       string `json:"s3_path"`
}

func (md MetadataImportDatabaseBackup) Build(op operations.Operation) proto.Message {
	return &ssv1.ImportDatabaseBackupMetadata{
		ClusterId:    op.TargetID,
		DatabaseName: md.DatabaseName,
		S3Bucket:     md.S3Bucket,
		S3Path:       md.S3Path,
	}
}

func (md MetadataImportDatabaseBackup) GetClusterID() string {
	return md.ClusterID
}

func (md MetadataImportDatabaseBackup) GetDatabaseName() string {
	return md.DatabaseName
}

func (md MetadataImportDatabaseBackup) GetS3Bucket() string {
	return md.S3Bucket
}

func (md MetadataImportDatabaseBackup) GetS3Path() string {
	return md.S3Path
}

type ExportDatabaseBackupMetadata interface {
	GetClusterID() string
	GetDatabaseName() string
	GetS3Bucket() string
	GetS3Path() string
}

type MetadataExportDatabaseBackup struct {
	ClusterID    string `json:"cluster_id"`
	DatabaseName string `json:"database_name"`
	S3Bucket     string `json:"s3_bucket"`
	S3Path       string `json:"s3_path"`
}

func (md MetadataExportDatabaseBackup) Build(op operations.Operation) proto.Message {
	return &ssv1.ExportDatabaseBackupMetadata{
		ClusterId:    op.TargetID,
		DatabaseName: md.DatabaseName,
		S3Bucket:     md.S3Bucket,
		S3Path:       md.S3Path,
	}
}

func (md MetadataExportDatabaseBackup) GetClusterID() string {
	return md.ClusterID
}

func (md MetadataExportDatabaseBackup) GetDatabaseName() string {
	return md.DatabaseName
}

func (md MetadataExportDatabaseBackup) GetS3Bucket() string {
	return md.S3Bucket
}

func (md MetadataExportDatabaseBackup) GetS3Path() string {
	return md.S3Path
}

func (md MetadataDeleteDatabase) GetDatabaseName() string {
	return md.DatabaseName
}

type UserMetadata interface {
	GetUserName() string
}

type MetadataCreateUser struct {
	UserName string `json:"user_name"`
}

func (md MetadataCreateUser) Build(op operations.Operation) proto.Message {
	return &ssv1.CreateUserMetadata{
		ClusterId: op.TargetID,
		UserName:  md.UserName,
	}
}

func (md MetadataCreateUser) GetUserName() string {
	return md.UserName
}

type MetadataUpdateUser struct {
	UserName string `json:"user_name"`
}

func (md MetadataUpdateUser) Build(op operations.Operation) proto.Message {
	return &ssv1.UpdateUserMetadata{
		ClusterId: op.TargetID,
		UserName:  md.UserName,
	}
}

func (md MetadataUpdateUser) GetUserName() string {
	return md.UserName
}

type MetadataGrantPermission struct {
	UserName string `json:"user_name"`
}

func (md MetadataGrantPermission) Build(op operations.Operation) proto.Message {
	return &ssv1.UpdateUserMetadata{
		ClusterId: op.TargetID,
		UserName:  md.UserName,
	}
}

func (md MetadataGrantPermission) GetUserName() string {
	return md.UserName
}

type MetadataRevokePermission struct {
	UserName string `json:"user_name"`
}

func (md MetadataRevokePermission) Build(op operations.Operation) proto.Message {
	return &ssv1.UpdateUserMetadata{
		ClusterId: op.TargetID,
		UserName:  md.UserName,
	}
}

func (md MetadataRevokePermission) GetUserName() string {
	return md.UserName
}

type MetadataDeleteUser struct {
	UserName string `json:"user_name"`
}

func (md MetadataDeleteUser) Build(op operations.Operation) proto.Message {
	return &ssv1.DeleteUserMetadata{
		ClusterId: op.TargetID,
		UserName:  md.UserName,
	}
}
func (md MetadataDeleteUser) GetUserName() string {
	return md.UserName
}

type MetadataUpdateHosts struct {
	HostNames []string `json:"host_names"`
}

func (md MetadataUpdateHosts) Build(op operations.Operation) proto.Message {
	return &ssv1.UpdateClusterHostsMetadata{
		ClusterId: op.TargetID,
		HostNames: md.HostNames,
	}
}
