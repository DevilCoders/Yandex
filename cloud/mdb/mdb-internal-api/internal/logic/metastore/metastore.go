package metastore

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/metastore/models"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
)

//go:generate ../../../../scripts/mockgen.sh Metastore

type Metastore interface {
	MDBCluster(ctx context.Context, cid string) (models.MDBCluster, error)
	MDBClusters(ctx context.Context, folderExtID string, limit, offset int64) ([]models.MDBCluster, error)
	CreateMDBCluster(ctx context.Context, args CreateMDBClusterArgs) (operations.Operation, error)
	DeleteCluster(ctx context.Context, cid string) (operations.Operation, error)
}

type CreateMDBClusterArgs struct {
	FolderExtID        string
	Name               string
	Description        string
	Labels             map[string]string
	NetworkID          string
	SubnetIDs          []string
	SecurityGroupIDs   []string
	HostGroupIDs       []string
	ConfigSpec         models.MDBClusterSpec
	DeletionProtection bool
	Version            string
	MinServersPerZone  int64
	MaxServersPerZone  int64
}
