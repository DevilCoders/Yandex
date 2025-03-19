package mongomodels

import (
	"github.com/golang/protobuf/proto"

	mongov1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/mongodb/v1"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
)

const (
	OperationTypeClusterCreate operations.Type = "mongodb_cluster_create"
	OperationTypeClusterDelete operations.Type = "mongodb_cluster_delete"
	OperationTypeDatabaseAdd   operations.Type = "mongodb_database_add"
	OperationTypeUserCreate    operations.Type = "mongodb_user_create"
	OperationTypeUserModify    operations.Type = "mongodb_user_modify"
	OperationTypeUserDelete    operations.Type = "mongodb_user_delete"
	OperationTypeResetupHosts  operations.Type = "mongodb_cluster_resetup_hosts"
	OperationTypeRestartHosts  operations.Type = "mongodb_cluster_restart_hosts"
	OperationTypeStepdownHosts operations.Type = "mongodb_cluster_stepdown_hosts"
	OperationTypeDeleteBackup  operations.Type = "mongodb_backup_delete"
)

func init() {
	operations.Register(
		OperationTypeClusterCreate,
		"Create MongoDB cluster",
		MetadataCreateCluster{},
	)
	operations.Register(
		OperationTypeClusterDelete,
		"Delete MongoDB cluster",
		MetadataDeleteCluster{},
	)
	operations.Register(
		OperationTypeDatabaseAdd,
		"Add database to MongoDB cluster",
		MetadataCreateDatabase{},
	)
	operations.Register(
		OperationTypeUserCreate,
		"Create user in MongoDB cluster",
		MetadataCreateUser{},
	)
	operations.Register(
		OperationTypeUserModify,
		"Modify user in MongoDB cluster",
		MetadataModifyUser{},
	)
	operations.Register(
		OperationTypeUserDelete,
		"Delete user from MongoDB cluster",
		MetadataDeleteUser{},
	)
	operations.Register(
		OperationTypeResetupHosts,
		"Resetup given hosts",
		MetadataResetupHosts{},
	)
	operations.Register(
		OperationTypeRestartHosts,
		"Restart given hosts",
		MetadataRestartHosts{},
	)
	operations.Register(
		OperationTypeStepdownHosts,
		"Stepdown given hosts",
		MetadataStepdownHosts{},
	)
	operations.Register(
		OperationTypeDeleteBackup,
		"Delete given backup",
		MetadataDeleteBackup{},
	)
}

type MetadataCreateCluster struct{}

func (md MetadataCreateCluster) Build(op operations.Operation) proto.Message {
	return &mongov1.CreateClusterMetadata{
		ClusterId: op.TargetID,
	}
}

type MetadataDeleteCluster struct{}

func (md MetadataDeleteCluster) Build(op operations.Operation) proto.Message {
	return &mongov1.DeleteClusterMetadata{
		ClusterId: op.TargetID,
	}
}

type MetadataCreateDatabase struct {
	DatabaseName string `json:"database_name"`
}

func (md MetadataCreateDatabase) Build(op operations.Operation) proto.Message {
	return &mongov1.CreateDatabaseMetadata{
		ClusterId:    op.TargetID,
		DatabaseName: md.DatabaseName,
	}
}

type MetadataCreateUser struct {
	UserName string `json:"user_name"`
}

func (md MetadataCreateUser) Build(op operations.Operation) proto.Message {
	return &mongov1.CreateUserMetadata{
		ClusterId: op.TargetID,
		UserName:  md.UserName,
	}
}

type MetadataModifyUser struct {
	UserName string `json:"user_name"`
}

func (md MetadataModifyUser) Build(op operations.Operation) proto.Message {
	return &mongov1.UpdateUserMetadata{
		ClusterId: op.TargetID,
		UserName:  md.UserName,
	}
}

type MetadataDeleteUser struct {
	UserName string `json:"user_name"`
}

func (md MetadataDeleteUser) Build(op operations.Operation) proto.Message {
	return &mongov1.UpdateUserMetadata{
		ClusterId: op.TargetID,
		UserName:  md.UserName,
	}
}

type MetadataResetupHosts struct {
	HostNames []string `json:"host_names"`
}

func (md MetadataResetupHosts) Build(op operations.Operation) proto.Message {
	return &mongov1.ResetupHostsMetadata{
		ClusterId: op.TargetID,
		HostNames: md.HostNames,
	}
}

type MetadataRestartHosts struct {
	HostNames []string `json:"host_names"`
}

func (md MetadataRestartHosts) Build(op operations.Operation) proto.Message {
	return &mongov1.RestartHostsMetadata{
		ClusterId: op.TargetID,
		HostNames: md.HostNames,
	}
}

type MetadataStepdownHosts struct {
	HostNames []string `json:"host_names"`
}

func (md MetadataStepdownHosts) Build(op operations.Operation) proto.Message {
	return &mongov1.StepdownHostsMetadata{
		ClusterId: op.TargetID,
		HostNames: md.HostNames,
	}
}

type MetadataDeleteBackup struct {
	BackupID string `json:"backup_id"`
}

func (md MetadataDeleteBackup) Build(op operations.Operation) proto.Message {
	return &mongov1.DeleteBackupMetadata{
		BackupId: md.BackupID,
	}
}
