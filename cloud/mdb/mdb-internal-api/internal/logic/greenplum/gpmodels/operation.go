package gpmodels

import (
	"github.com/golang/protobuf/proto"
	timestamppb "google.golang.org/protobuf/types/known/timestamppb"

	gpv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/greenplum/v1"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/backups"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
)

const (
	OperationTypeClusterCreate  operations.Type = "greenplum_cluster_create"
	OperationTypeClusterRestore operations.Type = "greenplum_cluster_restore"
	OperationTypeClusterDelete  operations.Type = "greenplum_cluster_delete"

	OperationTypeClusterStart operations.Type = "greenplum_cluster_start"
	OperationTypeClusterStop  operations.Type = "greenplum_cluster_stop"

	OperationTypeClusterUpdateTLSCerts operations.Type = "greenplum_cluster_update_tls_certs"
	OperationTypeClusterUpdate         operations.Type = "greenplum_cluster_update"

	OperationTypeClusterModify  operations.Type = "greenplum_cluster_modify"
	OperationTypeMetadataUpdate operations.Type = "greenplum_metadata_update"

	OperationTypeRescheduleMaintenance operations.Type = "greenplum_cluster_reschedule_maintenance"

	OperationTypeHostCreate operations.Type = "greenplum_host_create"
)

func init() {
	operations.Register(
		OperationTypeClusterCreate,
		"Create Greenplum cluster",
		MetadataCreateCluster{},
	)

	operations.Register(
		OperationTypeClusterRestore,
		"Restore Greenplum cluster",
		MetadataRestoreCluster{},
	)

	operations.Register(
		OperationTypeClusterDelete,
		"Delete Greenplum cluster",
		MetadataDeleteCluster{},
	)

	operations.Register(
		OperationTypeClusterStart,
		"Start Greenplum cluster",
		MetadataStartCluster{},
	)

	operations.Register(
		OperationTypeClusterStop,
		"Stop Greenplum cluster",
		MetadataStopCluster{},
	)

	operations.Register(
		OperationTypeClusterUpdateTLSCerts,
		"Update TLS for Greenplum cluster",
		MetadataUpdateTLSCertsCluster{},
	)

	operations.Register(
		OperationTypeClusterUpdate,
		"Update the Greenplum cluster",
		MetadataUpdateCluster{},
	)

	operations.Register(
		OperationTypeClusterModify,
		"Modify the Greenplum cluster",
		MetadataModifyCluster{},
	)

	operations.Register(
		OperationTypeMetadataUpdate,
		"Update the Greenplum cluster metadata",
		MetadataModifyCluster{},
	)

	operations.Register(
		OperationTypeRescheduleMaintenance,
		"Reschedule maintenance in Greenplum cluster",
		RescheduleMaintenanceMetadata{},
	)

	operations.Register(
		OperationTypeHostCreate,
		"Add host to Greenplum cluster",
		MetadataCreateHost{},
	)
}

type MetadataCreateCluster struct{}

func (md MetadataCreateCluster) Build(op operations.Operation) proto.Message {
	return &gpv1.CreateClusterMetadata{
		ClusterId: op.TargetID,
	}
}

type MetadataRestoreCluster struct {
	SourceClusterID string `json:"source_cid"`
	BackupID        string `json:"backup_id"`
}

func (md MetadataRestoreCluster) Build(op operations.Operation) proto.Message {
	return &gpv1.RestoreClusterMetadata{
		ClusterId: op.TargetID,
		BackupId:  backups.EncodeGlobalBackupID(md.SourceClusterID, md.BackupID),
	}
}

type MetadataDeleteCluster struct{}

func (md MetadataDeleteCluster) Build(op operations.Operation) proto.Message {
	return &gpv1.DeleteClusterMetadata{
		ClusterId: op.TargetID,
	}
}

type MetadataStartCluster struct{}

func (md MetadataStartCluster) Build(op operations.Operation) proto.Message {
	return &gpv1.StartClusterMetadata{
		ClusterId: op.TargetID,
	}
}

type MetadataStopCluster struct{}

func (md MetadataStopCluster) Build(op operations.Operation) proto.Message {
	return &gpv1.StopClusterMetadata{
		ClusterId: op.TargetID,
	}
}

type MetadataUpdateTLSCertsCluster struct{}

func (md MetadataUpdateTLSCertsCluster) Build(op operations.Operation) proto.Message {
	return &gpv1.UpdateClusterMetadata{
		ClusterId: op.TargetID,
	}
}

type MetadataUpdateCluster struct{}

func (md MetadataUpdateCluster) Build(op operations.Operation) proto.Message {
	return &gpv1.UpdateClusterMetadata{
		ClusterId: op.TargetID,
	}
}

type MetadataModifyCluster struct{}

func (md MetadataModifyCluster) Build(op operations.Operation) proto.Message {
	return &gpv1.UpdateClusterMetadata{
		ClusterId: op.TargetID,
	}
}

type RescheduleMaintenanceMetadata struct {
	DelayedUntil *timestamppb.Timestamp `json:"delayed_until"`
}

func (md RescheduleMaintenanceMetadata) Build(op operations.Operation) proto.Message {
	return &gpv1.RescheduleMaintenanceMetadata{
		ClusterId:    op.TargetID,
		DelayedUntil: md.DelayedUntil,
	}
}

type MetadataCreateHost struct {
	HostNames []string `json:"host_names"`
}

func (md MetadataCreateHost) Build(op operations.Operation) proto.Message {
	return &gpv1.AddClusterHostsMetadata{
		ClusterId: op.TargetID,
	}
}
