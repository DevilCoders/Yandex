package pgmodels

import (
	"github.com/golang/protobuf/proto"

	pgv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/postgresql/v1"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
)

const (
	OperationTypeClusterCreate operations.Type = "postgresql_cluster_create"
)

func init() {
	operations.Register(
		OperationTypeClusterCreate,
		"Create PostgreSQL cluster",
		MetadataCreateCluster{},
	)
}

type MetadataCreateCluster struct{}

func (md MetadataCreateCluster) Build(op operations.Operation) proto.Message {
	return &pgv1.CreateClusterMetadata{
		ClusterId: op.TargetID,
	}
}
