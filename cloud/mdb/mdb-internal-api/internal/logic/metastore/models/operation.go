package models

import (
	"github.com/golang/protobuf/proto"

	msv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/metastore/v1"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
)

const (
	OperationTypeClusterCreate operations.Type = "metastore_cluster_create"
	OperationTypeClusterDelete operations.Type = "metastore_cluster_delete"

	PasswordLen = 20
)

func init() {
	operations.Register(
		OperationTypeClusterCreate,
		"Create Metastore cluster",
		MetadataCreateCluster{},
	)

	operations.Register(
		OperationTypeClusterDelete,
		"Delete Metastore cluster",
		MetadataDeleteCluster{},
	)

}

type MetadataCreateCluster struct{}

func (md MetadataCreateCluster) Build(op operations.Operation) proto.Message {
	return &msv1.CreateClusterMetadata{
		ClusterId: op.TargetID,
	}
}

type MetadataDeleteCluster struct{}

func (md MetadataDeleteCluster) Build(op operations.Operation) proto.Message {
	return &msv1.DeleteClusterMetadata{
		ClusterId: op.TargetID,
	}
}
