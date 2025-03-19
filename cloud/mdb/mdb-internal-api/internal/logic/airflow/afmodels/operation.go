package afmodels

import (
	"github.com/golang/protobuf/proto"

	afv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/airflow/v1"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
)

const (
	OperationTypeClusterCreate operations.Type = "airflow_cluster_create"
	OperationTypeClusterDelete operations.Type = "airflow_cluster_delete"
)

func init() {
	operations.Register(
		OperationTypeClusterCreate,
		"Create Managed Airflow cluster",
		MetadataCreateCluster{},
	)
	operations.Register(
		OperationTypeClusterDelete,
		"Delete Managed Airflow cluster",
		MetadataDeleteCluster{},
	)
}

type MetadataCreateCluster struct{}

func (md MetadataCreateCluster) Build(op operations.Operation) proto.Message {
	return &afv1.CreateClusterMetadata{
		ClusterId: op.TargetID,
	}
}

type MetadataDeleteCluster struct{}

func (md MetadataDeleteCluster) Build(op operations.Operation) proto.Message {
	return &afv1.DeleteClusterMetadata{
		ClusterId: op.TargetID,
	}
}
