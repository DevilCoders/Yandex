package metadb

import (
	"context"
	"io"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/ready"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/library/go/core/xerrors"
)

//go:generate ../../../scripts/mockgen.sh MetaDB

// Storage errors
var (
	ErrDataNotFound = xerrors.NewSentinel("data not found")
)

type Backup struct {
	BackupID string
	DataSize int64
}

type BackupStatus string

const (
	BackupStatusPlanned     = BackupStatus("PLANNED")
	BackupStatusCreating    = BackupStatus("CREATING")
	BackupStatusDone        = BackupStatus("DONE")
	BackupStatusObsolete    = BackupStatus("OBSOLETE")
	BackupStatusDeleting    = BackupStatus("DELETING")
	BackupStatusDeleted     = BackupStatus("DELETED")
	BackupStatusCreateError = BackupStatus("CREATE-ERROR")
	BackupStatusDeleteError = BackupStatus("DELETE-ERROR")
)

type ClusterType string

const (
	PostgresqlCluster    = ClusterType("postgresql_cluster")
	MysqlCluster         = ClusterType("mysql_cluster")
	MongodbCluster       = ClusterType("mongodb_cluster")
	ClickhouseCluster    = ClusterType("clickhouse_cluster")
	RedisCluster         = ClusterType("redis_cluster")
	ElasticSearchCluster = ClusterType("elasticsearch_cluster")
	SQLServerCluster     = ClusterType("sqlserver_cluster")
	HadoopCluster        = ClusterType("hadoop_cluster")
	KafkaCluster         = ClusterType("kafka_cluster")
	GreenplumCluster     = ClusterType("greenplum_cluster")
)

type Cluster struct {
	ID       string
	Type     ClusterType
	FolderID string
	CloudID  string
}

type ClickHouseCloudStorageDetails struct {
	CloudID            string `db:"cloud_id"`
	FolderID           string `db:"folder_id"`
	CloudProvider      string `db:"cloud_provider"`
	CloudRegion        string `db:"cloud_region"`
	Bucket             string `db:"bucket"`
	ResourcePresetType string `db:"resource_preset_type"`
}

// MetaDB is an billing interface to MetaDB
type MetaDB interface {
	io.Closer
	ready.Checker

	sqlutil.TxBinder

	ListBackups(ctx context.Context, cid string, fromTS, untilTS time.Time) ([]Backup, error)
	ClusterSpace(ctx context.Context, cid string) (int64, error)
	ClusterMeta(ctx context.Context, cid string) (Cluster, error)
	ClickHouseCloudStorageDetails(ctx context.Context, cid string) (ClickHouseCloudStorageDetails, error)
}
