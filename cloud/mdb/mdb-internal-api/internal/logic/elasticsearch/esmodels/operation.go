package esmodels

import (
	"time"

	"github.com/golang/protobuf/proto"
	"google.golang.org/protobuf/types/known/timestamppb"

	esv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/elasticsearch/v1"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
)

const (
	OperationTypeClusterCreate  operations.Type = "elasticsearch_cluster_create"
	OperationTypeClusterDelete  operations.Type = "elasticsearch_cluster_delete"
	OperationTypeClusterModify  operations.Type = "elasticsearch_cluster_modify"
	OperationTypeMetadataUpdate operations.Type = "elasticsearch_metadata_update"
	OperationTypeClusterStart   operations.Type = "elasticsearch_cluster_start"
	OperationTypeClusterStop    operations.Type = "elasticsearch_cluster_stop"
	OperationTypeClusterBackup  operations.Type = "elasticsearch_cluster_create_backup"
	OperationTypeClusterRestore operations.Type = "elasticsearch_cluster_restore"

	OperationTypeUserCreate operations.Type = "elasticsearch_user_create"
	OperationTypeUserModify operations.Type = "elasticsearch_user_modify"
	OperationTypeUserDelete operations.Type = "elasticsearch_user_delete"

	OperationTypeHostCreate operations.Type = "elasticsearch_host_create"
	OperationTypeHostDelete operations.Type = "elasticsearch_host_delete"

	OperationTypeAddAuthProviders    operations.Type = "elasticsearch_auth_providers_add"
	OperationTypeUpdateAuthProviders operations.Type = "elasticsearch_auth_providers_update"
	OperationTypeDeleteAuthProviders operations.Type = "elasticsearch_auth_providers_delete"

	OperationTypeMaintenanceReschedule operations.Type = "elasticsearch_maintenance_reschedule"

	OperationTypeCreateExtension operations.Type = "elasticsearch_extension_create"
	OperationTypeUpdateExtension operations.Type = "elasticsearch_extension_update"
	OperationTypeDeleteExtension operations.Type = "elasticsearch_extension_delete"
)

func init() {
	operations.Register(
		OperationTypeClusterCreate,
		"Create ElasticSearch cluster",
		MetadataCreateCluster{},
	)
	operations.Register(
		OperationTypeClusterDelete,
		"Delete ElasticSearch cluster",
		MetadataDeleteCluster{},
	)
	operations.Register(
		OperationTypeClusterModify,
		"Modify ElasticSearch cluster",
		MetadataModifyCluster{},
	)
	operations.Register(
		OperationTypeMetadataUpdate,
		"Update ElasticSearch cluster metadata",
		MetadataModifyCluster{},
	)
	operations.Register(
		OperationTypeClusterStart,
		"Start ElasticSearch cluster",
		MetadataStartCluster{},
	)
	operations.Register(
		OperationTypeClusterStop,
		"Stop ElasticSearch cluster",
		MetadataStopCluster{},
	)
	operations.Register(
		OperationTypeUserCreate,
		"Create user in ElasticSearch cluster",
		MetadataCreateUser{},
	)

	operations.Register(
		OperationTypeUserModify,
		"Modify user in ElasticSearch cluster",
		MetadataModifyUser{},
	)

	operations.Register(
		OperationTypeUserDelete,
		"Delete user from ElasticSearch cluster",
		MetadataDeleteUser{},
	)

	operations.Register(
		OperationTypeHostCreate,
		"Add datanode host to ElasticSearch cluster",
		MetadataCreateHost{},
	)

	operations.Register(
		OperationTypeHostDelete,
		"Delete datanode host from ElasticSearch cluster",
		MetadataDeleteHost{},
	)

	operations.Register(
		OperationTypeAddAuthProviders,
		"Add authentication providers to ElasticSearch cluster",
		MetadataAddAuthProviders{},
	)

	operations.Register(
		OperationTypeUpdateAuthProviders,
		"Update authentication providers in ElasticSearch cluster",
		MetadataUpdateAuthProviders{},
	)

	operations.Register(
		OperationTypeDeleteAuthProviders,
		"Delete authentication providers from ElasticSearch cluster",
		MetadataDeleteAuthProviders{},
	)

	operations.Register(
		OperationTypeClusterBackup,
		"Create a backup for ElasticSearch cluster",
		MetadataBackupCluster{},
	)

	operations.Register(
		OperationTypeClusterRestore,
		"Restore ElasticSearch cluster from backup",
		MetadataRestoreCluster{},
	)

	operations.Register(
		OperationTypeMaintenanceReschedule,
		"Reschedule maintenance in ElasticSearch cluster",
		MetadataRescheduleMaintenance{},
	)

	operations.Register(
		OperationTypeCreateExtension,
		"Create extension in ElasticSearch cluster",
		MetadataCreateExtension{},
	)

	operations.Register(
		OperationTypeUpdateExtension,
		"Update extension in ElasticSearch cluster",
		MetadataUpdateExtension{},
	)

	operations.Register(
		OperationTypeDeleteExtension,
		"Delete extension in ElasticSearch cluster",
		MetadataDeleteExtension{},
	)
}

type MetadataCreateCluster struct{}

func (md MetadataCreateCluster) Build(op operations.Operation) proto.Message {
	return &esv1.CreateClusterMetadata{
		ClusterId: op.TargetID,
	}
}

type MetadataDeleteCluster struct{}

func (md MetadataDeleteCluster) Build(op operations.Operation) proto.Message {
	return &esv1.DeleteClusterMetadata{
		ClusterId: op.TargetID,
	}
}

type MetadataModifyCluster struct{}

func (md MetadataModifyCluster) Build(op operations.Operation) proto.Message {
	return &esv1.UpdateClusterMetadata{
		ClusterId: op.TargetID,
	}
}

type MetadataStartCluster struct{}

func (md MetadataStartCluster) Build(op operations.Operation) proto.Message {
	return &esv1.StartClusterMetadata{
		ClusterId: op.TargetID,
	}
}

type MetadataStopCluster struct{}

func (md MetadataStopCluster) Build(op operations.Operation) proto.Message {
	return &esv1.StopClusterMetadata{
		ClusterId: op.TargetID,
	}
}

type MetadataBackupCluster struct{}

func (md MetadataBackupCluster) Build(op operations.Operation) proto.Message {
	return &esv1.BackupClusterMetadata{
		ClusterId: op.TargetID,
	}
}

type MetadataRestoreCluster struct {
	BackupID string
}

func (md MetadataRestoreCluster) Build(op operations.Operation) proto.Message {
	return &esv1.RestoreClusterMetadata{
		ClusterId: op.TargetID,
		BackupId:  md.BackupID,
	}
}

type MetadataUser interface {
	GetUsername() string
}

type MetadataCreateUser struct {
	UserName string `json:"user_name"`
}

func (md MetadataCreateUser) GetUsername() string { return md.UserName }
func (md MetadataCreateUser) Build(op operations.Operation) proto.Message {
	return &esv1.CreateUserMetadata{
		ClusterId: op.TargetID,
		UserName:  md.UserName,
	}
}

type MetadataModifyUser struct {
	UserName string `json:"user_name"`
}

func (md MetadataModifyUser) GetUsername() string { return md.UserName }
func (md MetadataModifyUser) Build(op operations.Operation) proto.Message {
	return &esv1.UpdateUserMetadata{
		ClusterId: op.TargetID,
		UserName:  md.UserName,
	}
}

type MetadataDeleteUser struct {
	UserName string `json:"user_name"`
}

func (md MetadataDeleteUser) GetUsername() string { return md.UserName }
func (md MetadataDeleteUser) Build(op operations.Operation) proto.Message {
	return &esv1.DeleteUserMetadata{
		ClusterId: op.TargetID,
		UserName:  md.UserName,
	}
}

type MetadataCreateHost struct {
	HostNames []string `json:"host_names"`
}

func (md MetadataCreateHost) Build(op operations.Operation) proto.Message {
	return &esv1.AddClusterHostsMetadata{
		ClusterId: op.TargetID,
		HostNames: md.HostNames,
	}
}

type MetadataDeleteHost struct {
	HostNames []string `json:"host_names"`
}

func (md MetadataDeleteHost) Build(op operations.Operation) proto.Message {
	return &esv1.DeleteClusterHostsMetadata{
		ClusterId: op.TargetID,
		HostNames: md.HostNames,
	}
}

type MetadataAddAuthProviders struct {
	Names []string `json:"names"`
}

func (md MetadataAddAuthProviders) Build(op operations.Operation) proto.Message {
	return &esv1.AddAuthProvidersMetadata{
		ClusterId: op.TargetID,
		Names:     md.Names,
	}
}

type MetadataUpdateAuthProviders struct {
	Names []string `json:"names"`
}

func (md MetadataUpdateAuthProviders) Build(op operations.Operation) proto.Message {
	return &esv1.UpdateAuthProvidersMetadata{
		ClusterId: op.TargetID,
		Names:     md.Names,
	}
}

type MetadataDeleteAuthProviders struct {
	Names []string `json:"names"`
}

func (md MetadataDeleteAuthProviders) Build(op operations.Operation) proto.Message {
	return &esv1.DeleteAuthProvidersMetadata{
		ClusterId: op.TargetID,
		Names:     md.Names,
	}
}

type MetadataRescheduleMaintenance struct {
	DelayedUntil time.Time `json:"delayed_until"`
}

func (md MetadataRescheduleMaintenance) Build(op operations.Operation) proto.Message {
	return &esv1.RescheduleMaintenanceMetadata{
		ClusterId:    op.TargetID,
		DelayedUntil: timestamppb.New(md.DelayedUntil),
	}
}

type MetadataCreateExtension struct {
	ExtensionID string `json:"extension_id"`
}

func (md MetadataCreateExtension) Build(op operations.Operation) proto.Message {
	return &esv1.CreateExtensionMetadata{
		ClusterId:   op.TargetID,
		ExtensionId: md.ExtensionID,
	}
}

type MetadataDeleteExtension struct {
	ExtensionID string `json:"extension_id"`
}

func (md MetadataDeleteExtension) Build(op operations.Operation) proto.Message {
	return &esv1.DeleteExtensionMetadata{
		ClusterId:   op.TargetID,
		ExtensionId: md.ExtensionID,
	}
}

type MetadataUpdateExtension struct {
	ExtensionID string `json:"extension_id"`
}

func (md MetadataUpdateExtension) Build(op operations.Operation) proto.Message {
	return &esv1.UpdateExtensionMetadata{
		ClusterId:   op.TargetID,
		ExtensionId: md.ExtensionID,
	}
}
