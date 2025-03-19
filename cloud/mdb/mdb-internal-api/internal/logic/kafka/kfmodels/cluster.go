package kfmodels

import (
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/secret"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/environment"
)

const (
	CompressionTypeUnspecified  string = ""
	CompressionTypeUncompressed string = "uncompressed"
	CompressionTypeZstd         string = "zstd"
	CompressionTypeLz4          string = "lz4"
	CompressionTypeSnappy       string = "snappy"
	CompressionTypeGzip         string = "gzip"
	CompressionTypeProducer     string = "producer"
)

type MDBCluster struct {
	clusters.ClusterExtended
	Config MDBClusterSpec
}

type DataCloudCluster struct {
	clusters.ClusterExtended
	Version               string
	RegionID              string
	CloudType             environment.CloudType
	ConnectionInfo        ConnectionInfo
	PrivateConnectionInfo PrivateConnectionInfo
	Resources             DataCloudResources
	Access                clusters.Access
	Encryption            clusters.Encryption
}

const (
	KafkaClusterFeatureFlag        string = "MDB_KAFKA_CLUSTER"
	KafkaAllowNonHAOnHGFeatureFlag string = "MDB_KAFKA_ALLOW_NON_HA_ON_HG"
	KafkaAllowUpgradeFeatureFlag   string = "MDB_KAFKA_ALLOW_UPGRADE"
)

type ConnectionInfo struct {
	ConnectionString string
	User             string
	Password         secret.String
}

type PrivateConnectionInfo struct {
	ConnectionString string
	User             string
	Password         secret.String
}

type DataCloudResources struct {
	ResourcePresetID optional.String
	DiskSize         optional.Int64
	BrokerCount      optional.Int64
	ZoneCount        optional.Int64
}
