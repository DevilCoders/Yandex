package afmodels

import (
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
)

type MDBCluster struct {
	clusters.ClusterExtended
	Config    ClusterConfigSpec
	SubnetIDs []string
	CodeSync  CodeSyncConfig
}

type CodeSyncConfig struct {
	Bucket string
}

const (
	AirflowClusterFeatureFlag string = "MDB_AIRFLOW_CLUSTER"
)
