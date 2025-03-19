package gpmodels

import (
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
)

const (
	GreenplumClusterFeatureFlag                   string = "MDB_GREENPLUM_CLUSTER"
	GreenplumClusterAllowLowMemSegmentFeatureFlag string = "MDB_GREENPLUM_ALLOW_LOW_MEM"
)

type Cluster struct {
	clusters.ClusterExtended
	Config           ClusterConfig
	MasterConfig     MasterConfig
	SegmentConfig    SegmentConfig
	ClusterConfig    ClusterConfigSet
	ConfigSpec       ClusterGpConfigSet
	MasterHostCount  int64
	SegmentHostCount int64
	SegmentInHost    int64
	UserName         string
}
