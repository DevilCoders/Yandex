package models

import (
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
)

const (
	MetastoreClusterFeatureFlag string = "MDB_METASTORE_ALPHA"
)

type MDBCluster struct {
	clusters.ClusterExtended
	Config MDBClusterSpec
}
