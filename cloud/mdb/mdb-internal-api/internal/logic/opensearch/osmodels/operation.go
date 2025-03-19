package osmodels

import (
	"time"

	"github.com/golang/protobuf/proto"
	"google.golang.org/protobuf/types/known/timestamppb"

	protov1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/opensearch/v1"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
)

const (
	OperationTypeClusterCreate  operations.Type = "opensearch_cluster_create"
	OperationTypeClusterDelete  operations.Type = "opensearch_cluster_delete"
	OperationTypeClusterModify  operations.Type = "opensearch_cluster_modify"
	OperationTypeMetadataUpdate operations.Type = "opensearch_metadata_update"
	OperationTypeClusterStart   operations.Type = "opensearch_cluster_start"
	OperationTypeClusterStop    operations.Type = "opensearch_cluster_stop"
	OperationTypeClusterBackup  operations.Type = "opensearch_cluster_create_backup"
	OperationTypeClusterRestore operations.Type = "opensearch_cluster_restore"

	OperationTypeHostCreate operations.Type = "opensearch_host_create"
	OperationTypeHostDelete operations.Type = "opensearch_host_delete"

	OperationTypeAddAuthProviders    operations.Type = "opensearch_auth_providers_add"
	OperationTypeUpdateAuthProviders operations.Type = "opensearch_auth_providers_update"
	OperationTypeDeleteAuthProviders operations.Type = "opensearch_auth_providers_delete"

	OperationTypeMaintenanceReschedule operations.Type = "opensearch_maintenance_reschedule"

	OperationTypeCreateExtension operations.Type = "opensearch_extension_create"
	OperationTypeUpdateExtension operations.Type = "opensearch_extension_update"
	OperationTypeDeleteExtension operations.Type = "opensearch_extension_delete"
)

func init() {
	operations.Register(
		OperationTypeClusterCreate,
		"Create OpenSearch cluster",
		MetadataCreateCluster{},
	)
	operations.Register(
		OperationTypeClusterDelete,
		"Delete OpenSearch cluster",
		MetadataDeleteCluster{},
	)
	operations.Register(
		OperationTypeClusterModify,
		"Modify OpenSearch cluster",
		MetadataModifyCluster{},
	)
	operations.Register(
		OperationTypeMetadataUpdate,
		"Update OpenSearch cluster metadata",
		MetadataModifyCluster{},
	)
	operations.Register(
		OperationTypeClusterStart,
		"Start OpenSearch cluster",
		MetadataStartCluster{},
	)
	operations.Register(
		OperationTypeClusterStop,
		"Stop OpenSearch cluster",
		MetadataStopCluster{},
	)

	operations.Register(
		OperationTypeHostCreate,
		"Add datanode host to OpenSearch cluster",
		MetadataCreateHost{},
	)

	operations.Register(
		OperationTypeHostDelete,
		"Delete datanode host from OpenSearch cluster",
		MetadataDeleteHost{},
	)

	operations.Register(
		OperationTypeAddAuthProviders,
		"Add authentication providers to OpenSearch cluster",
		MetadataAddAuthProviders{},
	)

	operations.Register(
		OperationTypeUpdateAuthProviders,
		"Update authentication providers in OpenSearch cluster",
		MetadataUpdateAuthProviders{},
	)

	operations.Register(
		OperationTypeDeleteAuthProviders,
		"Delete authentication providers from OpenSearch cluster",
		MetadataDeleteAuthProviders{},
	)

	operations.Register(
		OperationTypeClusterBackup,
		"Create a backup for OpenSearch cluster",
		MetadataBackupCluster{},
	)

	operations.Register(
		OperationTypeClusterRestore,
		"Restore OpenSearch cluster from backup",
		MetadataRestoreCluster{},
	)

	operations.Register(
		OperationTypeMaintenanceReschedule,
		"Reschedule maintenance in OpenSearch cluster",
		MetadataRescheduleMaintenance{},
	)

	operations.Register(
		OperationTypeCreateExtension,
		"Create extension in OpenSearch cluster",
		MetadataCreateExtension{},
	)

	operations.Register(
		OperationTypeUpdateExtension,
		"Update extension in OpenSearch cluster",
		MetadataUpdateExtension{},
	)

	operations.Register(
		OperationTypeDeleteExtension,
		"Delete extension in OpenSearch cluster",
		MetadataDeleteExtension{},
	)
}

type MetadataCreateCluster struct{}

func (md MetadataCreateCluster) Build(op operations.Operation) proto.Message {
	return &protov1.CreateClusterMetadata{
		ClusterId: op.TargetID,
	}
}

type MetadataDeleteCluster struct{}

func (md MetadataDeleteCluster) Build(op operations.Operation) proto.Message {
	return &protov1.DeleteClusterMetadata{
		ClusterId: op.TargetID,
	}
}

type MetadataModifyCluster struct{}

func (md MetadataModifyCluster) Build(op operations.Operation) proto.Message {
	return &protov1.UpdateClusterMetadata{
		ClusterId: op.TargetID,
	}
}

type MetadataStartCluster struct{}

func (md MetadataStartCluster) Build(op operations.Operation) proto.Message {
	return &protov1.StartClusterMetadata{
		ClusterId: op.TargetID,
	}
}

type MetadataStopCluster struct{}

func (md MetadataStopCluster) Build(op operations.Operation) proto.Message {
	return &protov1.StopClusterMetadata{
		ClusterId: op.TargetID,
	}
}

type MetadataBackupCluster struct{}

func (md MetadataBackupCluster) Build(op operations.Operation) proto.Message {
	return &protov1.BackupClusterMetadata{
		ClusterId: op.TargetID,
	}
}

type MetadataRestoreCluster struct {
	BackupID string
}

func (md MetadataRestoreCluster) Build(op operations.Operation) proto.Message {
	return &protov1.RestoreClusterMetadata{
		ClusterId: op.TargetID,
		BackupId:  md.BackupID,
	}
}

type MetadataCreateHost struct {
	HostNames []string `json:"host_names"`
}

func (md MetadataCreateHost) Build(op operations.Operation) proto.Message {
	return &protov1.AddClusterHostsMetadata{
		ClusterId: op.TargetID,
		HostNames: md.HostNames,
	}
}

type MetadataDeleteHost struct {
	HostNames []string `json:"host_names"`
}

func (md MetadataDeleteHost) Build(op operations.Operation) proto.Message {
	return &protov1.DeleteClusterHostsMetadata{
		ClusterId: op.TargetID,
		HostNames: md.HostNames,
	}
}

type MetadataAddAuthProviders struct {
	Names []string `json:"names"`
}

func (md MetadataAddAuthProviders) Build(op operations.Operation) proto.Message {
	return &protov1.AddAuthProvidersMetadata{
		ClusterId: op.TargetID,
		Names:     md.Names,
	}
}

type MetadataUpdateAuthProviders struct {
	Names []string `json:"names"`
}

func (md MetadataUpdateAuthProviders) Build(op operations.Operation) proto.Message {
	return &protov1.UpdateAuthProvidersMetadata{
		ClusterId: op.TargetID,
		Names:     md.Names,
	}
}

type MetadataDeleteAuthProviders struct {
	Names []string `json:"names"`
}

func (md MetadataDeleteAuthProviders) Build(op operations.Operation) proto.Message {
	return &protov1.DeleteAuthProvidersMetadata{
		ClusterId: op.TargetID,
		Names:     md.Names,
	}
}

type MetadataRescheduleMaintenance struct {
	DelayedUntil time.Time `json:"delayed_until"`
}

func (md MetadataRescheduleMaintenance) Build(op operations.Operation) proto.Message {
	return &protov1.RescheduleMaintenanceMetadata{
		ClusterId:    op.TargetID,
		DelayedUntil: timestamppb.New(md.DelayedUntil),
	}
}

type MetadataCreateExtension struct {
	ExtensionID string `json:"extension_id"`
}

func (md MetadataCreateExtension) Build(op operations.Operation) proto.Message {
	return &protov1.CreateExtensionMetadata{
		ClusterId:   op.TargetID,
		ExtensionId: md.ExtensionID,
	}
}

type MetadataDeleteExtension struct {
	ExtensionID string `json:"extension_id"`
}

func (md MetadataDeleteExtension) Build(op operations.Operation) proto.Message {
	return &protov1.DeleteExtensionMetadata{
		ClusterId:   op.TargetID,
		ExtensionId: md.ExtensionID,
	}
}

type MetadataUpdateExtension struct {
	ExtensionID string `json:"extension_id"`
}

func (md MetadataUpdateExtension) Build(op operations.Operation) proto.Message {
	return &protov1.UpdateExtensionMetadata{
		ClusterId:   op.TargetID,
		ExtensionId: md.ExtensionID,
	}
}
