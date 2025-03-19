package chmodels

import (
	fieldmask_utils "github.com/mennanov/fieldmask-utils"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/secret"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
)

type Cluster struct {
	clusters.ClusterExtended
	CloudType        string
	RegionID         string
	Version          string
	ServiceAccountID optional.String
	MaintenanceInfo  clusters.MaintenanceInfo
}

type MDBCluster struct {
	Cluster
	Config ClusterConfig
}

type DataCloudCluster struct {
	Cluster
	ConnectionInfo ConnectionInfo
	Resources      DataCloudResources
	Access         clusters.Access
	Encryption     clusters.Encryption
	NetworkID      string
}

func NewCHClusterPageToken(cls []Cluster, expectedPageSize int64) clusters.ClusterPageToken {
	actualSize := int64(len(cls))

	var lastName string
	if actualSize > 0 {
		lastName = cls[actualSize-1].Name
	}

	return clusters.NewClusterPageToken(actualSize, expectedPageSize, lastName)
}

func NewMDBClusterPageToken(cls []MDBCluster, expectedPageSize int64) clusters.ClusterPageToken {
	c := make([]Cluster, len(cls))
	for i := range cls {
		c[i] = cls[i].Cluster
	}

	return NewCHClusterPageToken(c, expectedPageSize)
}

func NewDataCloudClusterPageToken(cls []DataCloudCluster, expectedPageSize int64) clusters.ClusterPageToken {
	c := make([]Cluster, len(cls))
	for i := range cls {
		c[i] = cls[i].Cluster
	}

	return NewCHClusterPageToken(c, expectedPageSize)
}

type ConnectionInfo struct {
	Hostname string
	Username string
	Password secret.String

	HTTPSPort     int64
	TCPPortSecure int64

	NativeProtocol string
	HTTPSURI       string
	JDBCURI        string
	ODBCURI        string
}

type DataCloudResources struct {
	ResourcePresetID optional.String
	DiskSize         optional.Int64
	ReplicaCount     optional.Int64
	ShardCount       optional.Int64
}

type DataCloudConfigSpecUpdate struct {
	Version   optional.String
	Resources DataCloudResources
	Access    clusters.Access
}

type MDBConfigSpecUpdate struct {
	MDBClusterSpec
	FieldMask fieldmask_utils.FieldFilter
}
