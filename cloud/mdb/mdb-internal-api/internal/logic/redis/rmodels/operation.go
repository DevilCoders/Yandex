package rmodels

import (
	"github.com/golang/protobuf/proto"

	redisv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/redis/v1"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
)

const (
	OperationTypeStartFailover operations.Type = "redis_cluster_start_failover"
	OperationTypeClusterStart  operations.Type = "redis_cluster_start"
	OperationTypeClusterStop   operations.Type = "redis_cluster_stop"
	OperationTypeClusterMove   operations.Type = "redis_cluster_move"
	OperationTypeRebalance     operations.Type = "redis_cluster_rebalance"
	OperationTypeClusterBackup operations.Type = "redis_cluster_create_backup"
	OperationTypeHostCreate    operations.Type = "redis_host_create"
)

func init() {
	operations.Register(
		OperationTypeStartFailover,
		"Start manual failover on Redis cluster",
		MetadataFailoverHosts{},
	)
	operations.Register(
		OperationTypeClusterStart,
		"Start Redis cluster",
		MetadataStartCluster{},
	)
	operations.Register(
		OperationTypeClusterStop,
		"Stop Redis cluster",
		MetadataStopCluster{},
	)
	operations.Register(
		OperationTypeClusterMove,
		"Move Redis cluster",
		MetadataMoveCluster{},
	)
	operations.Register(
		OperationTypeRebalance,
		"Rebalance slot distribution in Redis cluster",
		MetadataRebalance{},
	)
	operations.Register(
		OperationTypeClusterBackup,
		"Create a backup for Redis cluster",
		MetadataBackupCluster{},
	)
	operations.Register(
		OperationTypeHostCreate,
		"Add hosts to Redis cluster",
		MetadataHostCreate{},
	)
}

type MetadataFailoverHosts struct {
	HostNames []string `json:"host_names"`
}

func (md MetadataFailoverHosts) Build(op operations.Operation) proto.Message {
	return &redisv1.StartClusterFailoverMetadata{
		ClusterId: op.TargetID,
		HostNames: md.HostNames,
	}
}

type MetadataStartCluster struct{}

func (md MetadataStartCluster) Build(op operations.Operation) proto.Message {
	return &redisv1.StartClusterMetadata{
		ClusterId: op.TargetID,
	}
}

type MetadataStopCluster struct{}

func (md MetadataStopCluster) Build(op operations.Operation) proto.Message {
	return &redisv1.StopClusterMetadata{
		ClusterId: op.TargetID,
	}
}

type MetadataMoveCluster struct {
	SourceFolderID      string `json:"source_folder_id"`
	DestinationFolderID string `json:"destination_folder_id"`
}

func (md MetadataMoveCluster) Build(op operations.Operation) proto.Message {
	return &redisv1.MoveClusterMetadata{
		ClusterId:           op.TargetID,
		SourceFolderId:      md.SourceFolderID,
		DestinationFolderId: md.DestinationFolderID,
	}
}

type MetadataRebalance struct{}

func (md MetadataRebalance) Build(op operations.Operation) proto.Message {
	return &redisv1.RebalanceClusterMetadata{
		ClusterId: op.TargetID,
	}
}

type MetadataBackupCluster struct{}

func (md MetadataBackupCluster) Build(op operations.Operation) proto.Message {
	return &redisv1.BackupClusterMetadata{
		ClusterId: op.TargetID,
	}
}

type MetadataHostCreate struct {
	HostNames []string `json:"host_names"`
}

func (md MetadataHostCreate) Build(op operations.Operation) proto.Message {
	return &redisv1.AddClusterHostsMetadata{
		ClusterId: op.TargetID,
		HostNames: md.HostNames,
	}
}
