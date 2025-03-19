package metadb

import (
	"context"
	"fmt"
	"io"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/ready"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/library/go/core/xerrors"
)

//go:generate ../../../scripts/mockgen.sh MetaDB

// Storage errors
var (
	ErrDataNotFound = xerrors.NewSentinel("data not found")
)

type ClusterStatus string

const (
	ClusterStatusRunning            = ClusterStatus("RUNNING")
	ClusterStatusModifying          = ClusterStatus("MODIFYING")
	ClusterStatusModifyError        = ClusterStatus("MODIFY-ERROR")
	ClusterStatusRestoringOnline    = ClusterStatus("RESTORING-ONLINE")
	ClusterStatusRestoreOnlineError = ClusterStatus("RESTORE-ONLINE-ERROR")
)

var (
	ApplicableClusterStatuses = []ClusterStatus{ClusterStatusRunning, ClusterStatusModifying, ClusterStatusModifyError, ClusterStatusRestoringOnline, ClusterStatusRestoreOnlineError}
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

var clusterTypeMap = map[ClusterType]struct{}{
	PostgresqlCluster:    {},
	MysqlCluster:         {},
	MongodbCluster:       {},
	ClickhouseCluster:    {},
	RedisCluster:         {},
	ElasticSearchCluster: {},
	SQLServerCluster:     {},
	HadoopCluster:        {},
	KafkaCluster:         {},
	GreenplumCluster:     {},
}

// ParseClusterType from string
func ParseClusterType(str string) (ClusterType, error) {
	ct := ClusterType(str)
	if _, ok := clusterTypeMap[ct]; !ok {
		return "", fmt.Errorf("unknown cluster type: %s", ct)
	}
	return ct, nil
}

func ClusterTypesFromStrings(strs []string) ([]ClusterType, error) {
	ctypes := make([]ClusterType, len(strs))
	for i, str := range strs {
		ct, err := ParseClusterType(str)
		if err != nil {
			return nil, err
		}
		ctypes[i] = ct
	}
	return ctypes, nil
}

type SaltEnv string

const (
	SaltEnvDev         = SaltEnv("dev")
	SaltEvnQA          = SaltEnv("qa")
	SaltEnvProd        = SaltEnv("prod")
	SaltEnvComputeProd = SaltEnv("compute-prod")
)

const (
	MongodSubcluster          = "mongod_subcluster"
	MongocfgSubcluster        = "mongocfg_subcluster"
	MongoinfraSubcluster      = "mongoinfra_subcluster"
	DefaultBackupDelaySeconds = 300
)

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

var (
	AllStatuses     = []BackupStatus{BackupStatusPlanned, BackupStatusCreating, BackupStatusDone, BackupStatusObsolete, BackupStatusDeleting, BackupStatusDeleted, BackupStatusCreateError, BackupStatusDeleteError}
	StatusesCreated = []BackupStatus{BackupStatusDone, BackupStatusObsolete, BackupStatusDeleting, BackupStatusDeleting, BackupStatusDeleted, BackupStatusDeleteError}
	StatusesActive  = []BackupStatus{BackupStatusDone}

	StatusesUpdateSize = map[BackupStatus]struct{}{
		BackupStatusDone:     {},
		BackupStatusObsolete: {},
	}
)

type BackupMethod string

const (
	BackupMethodFull        = BackupMethod("FULL")
	BackupMethodIncremental = BackupMethod("INCREMENTAL")
)

type BackupInitiator string

const (
	BackupInitiatorUser     = BackupInitiator("USER")
	BackupInitiatorSchedule = BackupInitiator("SCHEDULE")
)

// BackupBlank ...
type BackupBlank struct {
	ClusterType    ClusterType
	ClusterID      string
	SubClusterID   string
	SubClusterName string
	ShardID        optional.String
	ScheduledTS    time.Time
	SleepSeconds   int
}

// CreateBackupArgs ...
type CreateBackupArgs struct {
	BackupID           string
	ClusterType        ClusterType
	ClusterID          string
	SubClusterID       string
	ShardID            optional.String
	Initiator          BackupInitiator
	Method             BackupMethod
	Status             BackupStatus
	DelayedUntil       time.Time
	ScheduledAt        optional.Time
	DependsOnBackupIDs []string
}

// ImportBackupArgs ...
type ImportBackupArgs struct {
	BackupID           string
	ClusterID          string
	SubClusterID       string
	ShardID            optional.String
	Initiator          BackupInitiator
	Method             BackupMethod
	Status             BackupStatus
	DelayedUntil       time.Time
	CreatedAt          time.Time
	StartedAt          time.Time
	UpdatedAt          time.Time
	FinishedAt         time.Time
	ScheduledAt        optional.Time
	Metadata           BackupMetadata
	DependsOnBackupIDs []string
}

// Backup ...
type Backup struct {
	BackupID      string
	ClusterType   ClusterType
	ClusterID     string
	SubClusterID  string
	ShardID       optional.String
	Method        BackupMethod
	Initiator     BackupInitiator
	Status        BackupStatus
	CreatedAt     time.Time
	DelayedUntil  time.Time
	StartedAt     optional.Time
	UpdatedAt     optional.Time
	FinishedAt    optional.Time
	ScheduledDate optional.Time
	ShipmentID    optional.String
	Metadata      []byte
	Errors        Errors
}

func (b Backup) Empty() bool {
	return b.BackupID == ""
}

func FilterBackupsByStatus(backups []Backup, statuses map[BackupStatus]struct{}) []Backup {
	var filtered []Backup
	for i := range backups {
		if _, ok := statuses[backups[i].Status]; !ok {
			continue
		}
		filtered = append(filtered, backups[i])
	}
	return filtered
}

type Host struct {
	ClusterID    string
	SubClusterID string
	ShardID      optional.String
	FQDN         string
}

func FqdnsFromHosts(hosts []Host) []string {
	fqdns := make([]string, len(hosts))
	for i := range hosts {
		fqdns[i] = hosts[i].FQDN
	}
	return fqdns
}

type Shard struct {
	SubClusterID   string
	SubClusterName string
	ID             optional.String
	Name           optional.String
}

type Cluster struct {
	ClusterID   string
	Name        string
	Type        ClusterType
	Environment SaltEnv
}

type SubCluster struct {
	ClusterID    string
	SubClusterID string
	Name         string
}

type ComponentVersion struct {
	Component      string
	Edition        string
	MajorVersion   string
	MinorVersion   string
	PackageVersion string
}

type BackupMetadata interface {
	Marshal() ([]byte, error)
	ID() string
}

// MetaDB is an search-producer interface to MetaDB
type MetaDB interface {
	io.Closer
	ready.Checker

	sqlutil.TxBinder

	// GetBackupBlanks get list of backups that should be scheduled in given window
	GetBackupBlanks(ctx context.Context, clusterTypes []ClusterType, past, future time.Duration) ([]BackupBlank, error)

	// GetClustersForImport returns list of cluster ids to run import on (list size is less or equal to maxCount)
	GetClustersForImport(ctx context.Context, maxCount int, clusterTypes []ClusterType, interval time.Duration) ([]string, error)

	// UpdateImportHistory updates entry in import history table for given cid with an optional error
	UpdateImportHistory(ctx context.Context, cid string, err error, t time.Time) error

	// AddBackup creates new backup record
	AddBackup(ctx context.Context, createBackupArgs CreateBackupArgs) (Backup, error)

	// ObsoleteAutomatedBackups marks automated backups to be deleted
	ObsoleteAutomatedBackups(ctx context.Context, clusterTypes []ClusterType) (int64, error)
	SequentialObsoleteAutomatedBackups(ctx context.Context, clusterTypes []ClusterType) (int64, error)
	ObsoleteFailedBackups(ctx context.Context, clusterTypes []ClusterType, backupAge time.Duration) (int64, error)
	SequentialObsoleteFailedBackups(ctx context.Context, clusterTypes []ClusterType, backupAge time.Duration) (int64, error)

	// PurgeDeletedBackups ...
	PurgeDeletedBackups(ctx context.Context, clusterTypes []ClusterType, backupAge time.Duration) (int64, error)

	// ListHosts...
	ListHosts(ctx context.Context, clusterID string, subClusterID, shardID optional.String) (hosts []Host, err error)

	HostsPillarByPath(ctx context.Context, fqdns []string, path []string) (map[string]string, error)

	ImportBackup(ctx context.Context, args ImportBackupArgs) (Backup, error)

	ClusterBucket(ctx context.Context, cid string) (string, error)
	ClusterStatusIsIn(ctx context.Context, cid string, statuses []ClusterStatus) (bool, error)
	ClusterVersions(ctx context.Context, cid string) (map[string]ComponentVersion, error)
	ClusterVersionsAtTS(ctx context.Context, cid string, ts time.Time) (map[string]ComponentVersion, error)
	BackupServiceEnabled(ctx context.Context, cid string) (bool, error)
	SetBackupServiceEnabled(ctx context.Context, cid string, enabled bool) error
	ListBackups(ctx context.Context, cid string, subcid optional.String, shardID optional.String, statuses []BackupStatus, initiators []BackupInitiator) ([]Backup, error)
	ListShards(ctx context.Context, cid string) ([]Shard, error)
	ListSubClusters(ctx context.Context, cid string) ([]SubCluster, error)
	Cluster(ctx context.Context, cid string) (Cluster, error)
	// ListParentBackups returns info about backups that passed backup depend on
	ListParentBackups(ctx context.Context, backupID string) ([]Backup, error)

	// PlannedBackup fetches backup intended to create
	PlannedBackup(ctx context.Context) (Backup, error)

	// CompleteBackupCreationStart marks backup creation started
	CompleteBackupCreationStart(ctx context.Context, backupID, shipmentID string) error
	// FailBackupCreation marks backup creation failed
	FailBackupCreation(ctx context.Context, backupID string, errs Errors) error

	// CreatingBackup fetches backup currently creating
	CreatingBackup(ctx context.Context) (Backup, error)
	// CompleteBackupCreation marks backup creation completed
	CompleteBackupCreation(ctx context.Context, backupID string, finishTime optional.Time, metadata BackupMetadata) error

	// ObsoleteBackup fetches backup intended to delete
	ObsoleteBackup(ctx context.Context) (Backup, error)

	// CompleteBackupDeletionStart marks backup deletion started
	CompleteBackupDeletionStart(ctx context.Context, backupID, shipmentID string) error
	// FailBackupDeletion marks backup deletion failed
	FailBackupDeletion(ctx context.Context, backupID string, errs Errors) error

	// DeletingBackup fetches backup currently deleting
	DeletingBackup(ctx context.Context) (Backup, error)
	// CompleteBackupDeletion marks backup deletion completed
	CompleteBackupDeletion(ctx context.Context, backupID string) error

	// DelayPendingBackupUntil postpones backup job
	DelayPendingBackupUntil(ctx context.Context, backupID string, t time.Time, errs Errors) error

	// BackupSchedule returns info about existing managed backups and schedule
	BackupSchedule(ctx context.Context, cid string) ([]byte, error)

	// SetBackupSize updates backup data and journal size
	SetBackupSize(ctx context.Context, backupID string, data, journal int64, t time.Time) error

	// LockBackups locks and fetches locked backups
	LockBackups(ctx context.Context, cid string, subcid optional.String, shardID optional.String, statuses []BackupStatus, initiators []BackupInitiator, exceptIDS []string) ([]Backup, error)
}
