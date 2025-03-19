package metadb

import "time"

type ClusterIDs = []string
type ClusterIDsByCloudID = map[string]ClusterIDs

type ClusterType string

const (
	ClickhouseCluster = ClusterType("clickhouse_cluster")
	KafkaCluster      = ClusterType("kafka_cluster")
)

type Cluster struct {
	ID           string
	Name         string
	Type         ClusterType
	Environment  string
	LastActionAt time.Time
}
