package metadb

import (
	"context"
	"encoding/json"
	"io"
	"time"

	"github.com/opentracing/opentracing-go"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/ready"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	bmodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/backups"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	consolemodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/console"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/environment"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/pillars"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/quota"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/resources"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/tasks"
)

//go:generate ../../../scripts/mockgen.sh Backend

// Backend interface to metadb
type Backend interface {
	io.Closer
	ready.Checker

	sqlutil.TxBinder
	UpdateCloudQuota(ctx context.Context, cloudExtID string, quota Resources, reqID string) (Cloud, error)
	CloudByCloudID(ctx context.Context, cloudID string) (Cloud, error)
	CloudByCloudExtID(ctx context.Context, cloudExtID string) (Cloud, error)
	CreateCloud(ctx context.Context, cloudExtID string, quota Resources, reqID string) (Cloud, error)
	LockCloud(ctx context.Context, cloudID int64) (Cloud, error)
	UpdateCloudUsedQuota(ctx context.Context, cloudID int64, quota Resources, reqID string) (Cloud, error)
	DefaultFeatureFlags(ctx context.Context) ([]string, error)

	FolderCoordsByFolderExtID(ctx context.Context, folderExtID string) (FolderCoords, error)
	FolderCoordsByClusterID(ctx context.Context, cid string, vis models.Visibility) (FolderCoords, int64, clusters.Type, error)
	FolderCoordsByOperationID(ctx context.Context, opID string) (FolderCoords, error)
	CreateFolder(ctx context.Context, folderExtID string, cloudID int64) (FolderCoords, error)

	RegionByName(ctx context.Context, name string) (environment.Region, error)
	ListZones(ctx context.Context) ([]environment.Zone, error)
	ResourcePresetByExtID(ctx context.Context, extID string) (resources.Preset, error)
	GetResourcePreset(ctx context.Context, clusterType clusters.Type, role hosts.Role, flavorType optional.String,
		resourcePresetExtID, diskTypeExtID optional.String, generation optional.Int64, minCPU optional.Float64, zones, featureFlags, decommissionedResourcePresets []string) (resources.DefaultPreset, error)
	DiskTypes(ctx context.Context) (resources.DiskTypes, error)
	DiskTypeExtIDByResourcePreset(ctx context.Context, clusterType clusters.Type, role hosts.Role, resourcePreset string,
		zones []string, featureFlags []string) (string, error)
	DiskIOLimit(ctx context.Context, spaceLimit int64, diskType string, resourcePreset string) (int64, error)
	ValidResources(ctx context.Context, featureFlags []string, typ clusters.Type, role hosts.Role, resourcePresetExtID, diskTypeExtID, zoneID optional.String) ([]resources.Valid, error)

	ClusterRevisionByTime(ctx context.Context, cid string, time time.Time) (int64, error)
	LockCluster(ctx context.Context, cid, reqid string) (Cluster, error)
	CompleteClusterChange(ctx context.Context, cid string, revision int64) error
	ClusterByClusterID(ctx context.Context, cid string, vis models.Visibility) (Cluster, error)
	ClusterByClusterIDAtRevision(ctx context.Context, cid string, rev int64) (Cluster, error)
	CreateCluster(ctx context.Context, reqid string, args models.CreateClusterArgs) (Cluster, error)
	CreateSubCluster(ctx context.Context, args models.CreateSubClusterArgs) (SubCluster, error)
	DeleteSubCluster(ctx context.Context, args models.DeleteSubClusterArgs) error
	CreateKubernetesSubCluster(ctx context.Context, args models.CreateSubClusterArgs) (SubCluster, error)
	UpdatePillar(ctx context.Context, cid string, revision int64, pillar json.RawMessage, args map[string]interface{}) error
	AddPillar(ctx context.Context, cid string, revision int64, pillar json.RawMessage, args map[string]interface{}) error
	AddTargetPillar(ctx context.Context, targetID string, pillar json.RawMessage, args map[string]interface{}) error
	Clusters(ctx context.Context, args models.ListClusterArgs) ([]Cluster, error)
	ListClusterIDs(ctx context.Context, args models.ListClusterIDsArgs) ([]string, error)
	AddBackupSchedule(ctx context.Context, cid string, schedule bmodels.BackupSchedule, revision int64) error
	UpdateClusterName(ctx context.Context, cid string, name string, revision int64) error
	UpdateClusterDescription(ctx context.Context, cid string, description string, revision int64) error
	UpdateClusterFolder(ctx context.Context, cid string, folderID, revision int64) error
	SetLabelsOnCluster(ctx context.Context, cid string, labels map[string]string, revision int64) error
	UpdateDeletionProtection(ctx context.Context, cid string, deletionProtection bool, revision int64) error
	GetClusterQuotaUsage(ctx context.Context, cid string) (Resources, error)

	SubClustersByClusterID(ctx context.Context, cid string) ([]SubCluster, error)
	SubClustersByClusterIDAtRevision(ctx context.Context, cid string, rev int64) ([]SubCluster, error)
	AddSubClusterPillar(ctx context.Context, cid, subcid string, revision int64, pillar json.RawMessage) error
	UpdateSubClusterPillar(ctx context.Context, cid, subcid string, revision int64, pillar json.RawMessage) error

	AddClusterPillar(ctx context.Context, cid string, revision int64, pillar json.RawMessage) error
	UpdateClusterPillar(ctx context.Context, cid string, revision int64, pillar json.RawMessage) error

	ClusterTypePillar(ctx context.Context, typ clusters.Type, marshaller pillars.Marshaler) error
	AddHostPillar(ctx context.Context, cid string, fqdn string, revision int64, marshaller pillars.Marshaler) error
	UpdateHostPillar(ctx context.Context, cid string, fqdn string, revision int64, marshaller pillars.Marshaler) error
	HostPillar(ctx context.Context, fqdn string, marshaller pillars.Marshaler) error

	OperationByID(ctx context.Context, oid string) (operations.Operation, error)
	OperationIDByIdempotenceID(ctx context.Context, idempID, userID string, folderID int64) (string, []byte, error)
	OperationsByFolderID(ctx context.Context, folderID int64, args models.ListOperationsArgs) ([]operations.Operation, error)
	OperationsByClusterID(ctx context.Context, cid string, folderID int64, offset int64, pageSize int64) ([]operations.Operation, error)
	MostRecentInitiatedByUserOperationByClusterID(ctx context.Context, cid string) (operations.Operation, error)

	RunningTaskID(ctx context.Context, cid string) (string, error)
	FailedTaskID(ctx context.Context, cid string) (string, error)
	CreateTask(ctx context.Context, args tasks.CreateTaskArgs, tracingCarrier opentracing.TextMapCarrier) (operations.Operation, error)
	CreateFinishedTask(ctx context.Context, args tasks.CreateFinishedTaskArgs) (operations.Operation, error)
	CreateFinishedTaskAtCurrentRev(ctx context.Context, args tasks.CreateFinishedTaskArgs) (operations.Operation, error)

	CreateWorkerQueueEvent(ctx context.Context, taskID string, data []byte) error
	CreateSearchQueueDoc(ctx context.Context, doc []byte) error

	GetUsedResources(ctx context.Context, clouds []string) ([]consolemodels.UsedResources, error)
	GetResourcePresetsByClusterType(ctx context.Context, clusterType clusters.Type, featureFlags []string) ([]consolemodels.ResourcePreset, error)
	GetResourcePresetsByCloudRegion(ctx context.Context, cloudType environment.CloudType, region string, clusterType clusters.Type, featureFlags []string) ([]consolemodels.ResourcePreset, error)
	ClustersCountInFolder(ctx context.Context, folderID int64) (map[clusters.Type]int64, error)
	GetCloudsByClusterType(ctx context.Context, clusterType clusters.Type, pageSize, offset int64) ([]consolemodels.Cloud, int64, error)

	AddHost(ctx context.Context, args models.AddHostArgs) (hosts.Host, error)
	ModifyHost(ctx context.Context, args models.ModifyHostArgs) (hosts.Host, error)
	ModifyHostPublicIP(ctx context.Context, clusterID string, fqdn string, revision int64, assignPublicIP bool) (hosts.Host, error)
	DeleteHosts(ctx context.Context, clusterID string, fqdns []string, revision int64) ([]hosts.Host, error)
	ListHosts(ctx context.Context, clusterID string, offset int64, pageSize optional.Int64) (hosts []hosts.Host, nextPageToken int64, more bool, err error)
	HostsByClusterIDRoleAtRevision(ctx context.Context, clusterID string, role hosts.Role, rev int64) (hosts []hosts.Host, err error)
	HostsByClusterIDShardNameAtRevision(ctx context.Context, clusterID string, shardName string, rev int64) (hosts []hosts.Host, err error)
	HostsByShardID(ctx context.Context, shardID, clusterID string) ([]hosts.Host, error)

	ShardByShardID(ctx context.Context, shardid string) (Shard, error)
	ShardByShardName(ctx context.Context, shardName, clusterID string) (Shard, error)
	ShardsByClusterID(ctx context.Context, cid string) ([]Shard, error)
	CreateShard(ctx context.Context, args models.CreateShardArgs) (Shard, error)
	AddShardPillar(ctx context.Context, cid, shardid string, revision int64, pillar json.RawMessage) error
	UpdateShardPillar(ctx context.Context, cid, subcid string, revision int64, pillar json.RawMessage) error
	DeleteShard(ctx context.Context, cid, shardid string, revision int64) error

	KubernetesNodeGroup(ctx context.Context, subcid string) (KubernetesNodeGroup, error)

	ScheduleBackupForNow(ctx context.Context, backupID string, cid string, subcid string, shardID string, method bmodels.BackupMethod, metadata []byte) error
	BackupByID(ctx context.Context, backupID string) (bmodels.ManagedBackup, error)
	BackupsByClusterID(ctx context.Context, clusterID string) ([]bmodels.ManagedBackup, error)
	ClusterIDByBackupID(ctx context.Context, backupID string) (string, error)
	MarkBackupObsolete(ctx context.Context, backupID string) error

	ClusterUsesBackupService(ctx context.Context, cid string) (bool, error)
	GetClusterVersions(ctx context.Context, cid string) ([]consolemodels.Version, error)
	GetClusterVersionsAtRevision(ctx context.Context, cid string, rev int64) ([]consolemodels.Version, error)
	GetDefaultVersions(ctx context.Context, clusterType clusters.Type, env environment.SaltEnv, component string) ([]consolemodels.DefaultVersion, error)

	SetDefaultVersionCluster(ctx context.Context, cid string,
		ctype clusters.Type, env environment.SaltEnv, majorVersion string, edition string, revision int64) error

	AddDiskPlacementGroup(ctx context.Context, args models.AddDiskPlacementGroupArgs) (int64, error)
	AddDisk(ctx context.Context, args models.AddDiskArgs) (int64, error)

	MaintenanceInfoByClusterID(ctx context.Context, cid string) (clusters.MaintenanceInfo, error)
	SetMaintenanceWindowSettings(ctx context.Context, cid string, rev int64, mw clusters.MaintenanceWindow) error
	RescheduleMaintenanceTask(ctx context.Context, cid, configID string, maintenanceTime time.Time) error

	AddPlacementGroup(ctx context.Context, args models.AddPlacementGroupArgs) (int64, error)
}

// Resources ...
type Resources struct {
	CPU      float64 // Cores count
	GPU      int64   // Cores count
	Memory   int64   // Bytes
	SSDSpace int64   // Bytes
	HDDSpace int64   // Bytes
	Clusters int64   // Count
}

func (r *Resources) FromQuotaResources(resources quota.Resources) Resources {
	return Resources{
		CPU:      resources.CPU,
		GPU:      resources.GPU,
		Memory:   resources.Memory,
		SSDSpace: resources.SSDSpace,
		HDDSpace: resources.HDDSpace,
		Clusters: resources.Clusters,
	}
}

func (r *Resources) ToQuotaResources() quota.Resources {
	return quota.Resources{
		CPU:      r.CPU,
		GPU:      r.GPU,
		Memory:   r.Memory,
		SSDSpace: r.SSDSpace,
		HDDSpace: r.HDDSpace,
		Clusters: r.Clusters,
	}
}

// Cloud ...
type Cloud struct {
	CloudID      int64
	CloudExtID   string
	Quota        Resources
	Used         Resources
	FeatureFlags []string
}

type FolderCoords struct {
	CloudID     int64
	CloudExtID  string
	FolderID    int64
	FolderExtID string
}

type Cluster struct {
	clusters.Cluster
	Pillar json.RawMessage
}

type SubCluster struct {
	clusters.SubCluster
	Pillar json.RawMessage
}

type KubernetesNodeGroup struct {
	SubClusterID        string
	KubernetesClusterID string
	NodeGroupID         string
}

type Shard struct {
	clusters.Shard
	Pillar json.RawMessage
}
