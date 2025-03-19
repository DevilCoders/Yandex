package metadb

import (
	"encoding/json"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
)

// Operation models MDB operation info
type Operation struct {
	ID            string          `json:"id" yaml:"id"`
	FolderID      string          `json:"folder_id" yaml:"folder_id"`
	CreatedBy     string          `json:"created_by" yaml:"created_by"`
	ClusterID     string          `json:"cid" yaml:"cid"`
	WorkerID      string          `json:"worker_id" yaml:"worker_id"`
	CreatedAt     time.Time       `json:"createdate" yaml:"createdate"`
	Deadline      time.Time       `json:"deadline" yaml:"deadline"`
	StartedAt     time.Time       `json:"startdate" yaml:"startdate"`
	EndedAt       time.Time       `json:"enddate" yaml:"enddate"`
	TaskType      string          `json:"task_type" yaml:"task_type"`
	Args          interface{}     `json:"args" yaml:"args"`
	Changes       []Change        `json:"changes" yaml:"changes"`
	Comment       string          `json:"comment" yaml:"comment"`
	Result        bool            `json:"result" yaml:"result"`
	OperationType string          `json:"operation_type" yaml:"operation_type"`
	MetaData      json.RawMessage `json:"metadata" yaml:"metadata"`
}

// Change models one MDB operation change
type Change struct {
	TS     time.Time `json:"ts" yaml:"ts"`
	What   string    `json:"what" yaml:"what"`
	Status string    `json:"status" yaml:"status"`
}

// CidAndTimestamp cid and timestamp
type CidAndTimestamp struct {
	ClusterID string    `json:"cid" yaml:"cid"`
	TS        time.Time `json:"ts" yaml:"ts"`
}

// ClusterInfo ...
type ClusterInfo struct {
	PubKey   []byte        `json:"pkey" yaml:"pkey"`
	NetID    string        `json:"nid" yaml:"nid"`
	CType    ClusterType   `json:"ctype" yaml:"ctype"`
	Env      Environment   `json:"env" yaml:"env"`
	FolderID int64         `json:"folder_id" yaml:"folder_id"`
	Status   ClusterStatus `json:"status" yaml:"status"`
}

// ShardInfo ...
type ShardInfo struct {
	ShardName string `json:"sname" yaml:"sname"`
}

// Resources ...
type Resources struct {
	CPUCores      float64 `json:"cpu_cores" yaml:"cpu_cores"`
	MemoryBytes   int64   `json:"memory_bytes" yaml:"memory_bytes"`
	SSDBytes      int64   `json:"ssd_bytes" yaml:"ssd_bytes"`
	HDDBytes      int64   `json:"hdd_bytes" yaml:"hdd_bytes"`
	ClustersCount int64   `json:"clusters_count" yaml:"clusters_count"`
}

// Cloud ...
type Cloud struct {
	CloudID string
	Used    Resources
	Quota   Resources
}

// ResourcesChange ...
type ResourcesChange struct {
	CPUCores      optional.Float64
	MemoryBytes   optional.Int64
	SSDBytes      optional.Int64
	HDDBytes      optional.Int64
	ClustersCount optional.Int64
}

// WorkerQueueEvent is a worker_queue_event row
type WorkerQueueEvent struct {
	ID        int64
	Data      string
	CreatedAt time.Time
}

// ClusterRev is rev with it's status
type ClusterRev struct {
	ClusterID string
	Rev       int64
}

// Cluster ... info
type Cluster struct {
	Name        string
	Type        ClusterType
	Environment string
	Visible     bool
	Status      string
}

// Host is a host info about it
type Host struct {
	FQDN         string
	ClusterID    string
	SubClusterID string
	ShardID      optional.String
	Geo          string
	Roles        []string
	CreatedAt    time.Time
	DType        DiskType
}

type DiskType string

const (
	LocalSSD                = DiskType("local-ssd")
	LocalHDD                = DiskType("local-hdd")
	NetworkSSD              = DiskType("network-ssd")
	NetworkHDD              = DiskType("network-hdd")
	NetworkSSDNonReplicated = DiskType("network-ssd-nonreplicated")
	GP2                     = DiskType("gp2") // aws disk type
)

type ClusterType string

const (
	PostgresqlCluster    = ClusterType("postgresql_cluster")
	GreenplumCluster     = ClusterType("greenplum_cluster")
	MysqlCluster         = ClusterType("mysql_cluster")
	MongodbCluster       = ClusterType("mongodb_cluster")
	ClickhouseCluster    = ClusterType("clickhouse_cluster")
	RedisCluster         = ClusterType("redis_cluster")
	ElasticSearchCluster = ClusterType("elasticsearch_cluster")
	SQLServerCluster     = ClusterType("sqlserver_cluster")
	HadoopCluster        = ClusterType("hadoop_cluster")
	KafkaCluster         = ClusterType("kafka_cluster")
)

type CustomRole string

const (
	PostgresqlCascadeReplicaRole = CustomRole("PostgresqlCascadeReplicaRole")
	MysqlCascadeReplicaRole      = CustomRole("MysqlCascadeReplicaRole")
)

type Environment string

const (
	DevEnvironment         = Environment("dev")
	LoadEnvironment        = Environment("load")
	QaEnvironment          = Environment("qa")
	ProdEnvironment        = Environment("prod")
	ComputeProdEnvironment = Environment("compute-prod")
)

type ClusterStatus string

const (
	ClusterStatusRunning            = "RUNNING"
	ClusterStatusStopped            = "STOPPED"
	ClusterStatusModifyError        = "MODIFY-ERROR"
	ClusterStatusStopError          = "STOP-ERROR"
	ClusterStatusStartError         = "START-ERROR"
	ClusterStatusRestoreOnlineError = "RESTORE-ONLINE-ERROR"
)
