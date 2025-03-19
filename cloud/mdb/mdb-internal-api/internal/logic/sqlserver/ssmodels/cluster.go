package ssmodels

import (
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
)

const (
	SQLServerClusterFeatureFlag    string = "MDB_SQLSERVER_CLUSTER"
	SQLServerAllowDevFeatureFlag   string = "MDB_SQLSERVER_ALLOW_DEV"
	SQLServerAllow17_19FeatureFlag string = "MDB_SQLSERVER_ALLOW_17_19"
	SQLServerTwoNodeCluster        string = "MDB_SQLSERVER_TWO_NODE_CLUSTER"
	SQLServerDedicatedHostsFlag    string = "MDB_DEDICATED_HOSTS"
)

type Cluster struct {
	clusters.ClusterExtended
	Config           ClusterConfig
	SQLCollation     string
	ServiceAccountID string
}
