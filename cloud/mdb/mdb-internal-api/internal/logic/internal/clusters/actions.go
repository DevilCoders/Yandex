package clusters

import (
	"context"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/compute"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/sessions"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	clustermodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/console"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/environment"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts"
	modelsoptional "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/optional"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/pillars"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/resources"
)

// Creator allows creating clusters.
type Creator interface {
	baseModifier

	// CreateCluster with specified arguments
	CreateCluster(ctx context.Context, args models.CreateClusterArgs) (clustermodels.Cluster, []byte, error)

	AddClusterPillar(ctx context.Context, cid string, revision int64, pillar Pillar) error
	AddTargetPillar(ctx context.Context, cid string, pillar pillars.Marshaler) (string, error)

	DiskTypeExtIDByResourcePreset(ctx context.Context, clusterType clustermodels.Type, role hosts.Role, resourcePreset string,
		zones []string, featureFlags []string) (string, error)

	SetDefaultVersionCluster(ctx context.Context, cid string,
		ctype clustermodels.Type, env environment.SaltEnv, majorVersion string, edition string, revision int64) error

	EstimateBilling(ctx context.Context, folderID string, clusterType clustermodels.Type, hosts []HostBillingSpec, cloudType environment.CloudType) (console.BillingEstimate, error)
}

// Restorer allows restoring clusters.
type Restorer interface {
	Creator

	// TODO: may be remove the following functions from this interface and do required work inside operator
	ClusterByClusterID(ctx context.Context, cid string, typ clustermodels.Type, vis models.Visibility) (Cluster, error)
	ClusterAtTime(ctx context.Context, cid string, time time.Time, typ clustermodels.Type) (Cluster, error)
	ClusterAndResourcesAtTime(ctx context.Context, cid string, time time.Time, typ clustermodels.Type, role hosts.Role) (Cluster, models.ClusterResources, error)
	ClusterAndShardResourcesAtTime(ctx context.Context, cid string, time time.Time, typ clustermodels.Type, shardName string) (Cluster, models.ClusterResources, error)
	ShardResourcesAtRevision(ctx context.Context, shardName, clusterID string, rev int64) (models.ClusterResources, error)
}

// Reader allows reading info from cluster.
type Reader interface {
	ClusterByClusterID(ctx context.Context, cid string, typ clustermodels.Type, vis models.Visibility) (Cluster, error)
	ClusterExtendedByClusterID(ctx context.Context, cid string, typ clustermodels.Type, vis models.Visibility, session sessions.Session) (clustermodels.ClusterExtended, error)
	ClusterByClusterIDAtRevision(ctx context.Context, cid string, typ clustermodels.Type, rev int64) (Cluster, error)
	Clusters(ctx context.Context, args models.ListClusterArgs) ([]Cluster, error)
	ClustersExtended(ctx context.Context, args models.ListClusterArgs, session sessions.Session) ([]clustermodels.ClusterExtended, error)
	ClusterVersions(ctx context.Context, cid string) ([]console.Version, error)
	ClusterVersionsAtTime(ctx context.Context, cid string, t time.Time) ([]console.Version, error)
	ClusterUsesBackupService(ctx context.Context, cid string) (bool, error)

	SubClusterByRole(ctx context.Context, cid string, role hosts.Role, pillar pillars.Marshaler) (clustermodels.SubCluster, error)
	SubClusterByRoleAtRevision(ctx context.Context, cid string, role hosts.Role, pillar pillars.Marshaler, rev int64) (clustermodels.SubCluster, error)
	KubernetesSubClusters(ctx context.Context, cid string) ([]clustermodels.KubernetesSubCluster, error)

	AnyHost(ctx context.Context, clusterID string) (hosts.HostExtended, error)
	ListHosts(ctx context.Context, clusterID string, pageSize int64, offset int64) (hosts []hosts.HostExtended, nextPageToken int64, hasMore bool, err error)

	ShardByShardID(ctx context.Context, shardID string, pillar pillars.Marshaler) (clustermodels.Shard, error)
	ShardByShardName(ctx context.Context, shardName, clusterID string, pillar pillars.Marshaler) (clustermodels.Shard, error)
	ListShards(ctx context.Context, clusterID string) ([]clustermodels.Shard, error)
	ListShardsExtended(ctx context.Context, clusterID string) ([]clustermodels.ShardExtended, error)
	ListShardHosts(ctx context.Context, shardID, clusterID string) ([]hosts.HostExtended, error)

	ClusterTypePillar(ctx context.Context, typ clustermodels.Type, marshaller pillars.Marshaler) error
	HostPillar(ctx context.Context, fqdn string, marshaller pillars.Marshaler) error

	EstimateBilling(ctx context.Context, folderID string, clusterType clustermodels.Type, hosts []HostBillingSpec, cloudType environment.CloudType) (console.BillingEstimate, error)

	ResourcesByClusterIDRoleAtRevision(ctx context.Context, cid string, rev int64, role hosts.Role) (models.ClusterResources, error)
	ClusterAndResourcesAtTime(ctx context.Context, cid string, time time.Time, typ clustermodels.Type, role hosts.Role) (Cluster, models.ClusterResources, error)
	ResourcePresetFromDefaultConfig(clusterType clustermodels.Type, role hosts.Role) (resources.DefaultPreset, error)
	ValidateResources(ctx context.Context, session sessions.Session, typ clustermodels.Type, hostGroups ...HostGroup) (ResolvedHostGroups, bool, error)

	MaintenanceInfoByClusterID(ctx context.Context, cid string) (clustermodels.MaintenanceInfo, error)
}

// Modifier allows modifying existing cluster.
type Modifier interface {
	baseModifier

	UpdateClusterName(ctx context.Context, cluster Cluster, name string) error
	UpdateClusterFolder(ctx context.Context, cluster Cluster, folderExtID string) error
	// Changes require to run metadata operation
	ModifyClusterMetadata(ctx context.Context, cluster Cluster, name optional.String, labels modelsoptional.Labels) (bool, error)
	// Metadb-only changes
	ModifyClusterMetadataParameters(ctx context.Context, cluster Cluster, description optional.String, labels modelsoptional.Labels,
		deletionProtection optional.Bool, maintenanceWindow modelsoptional.MaintenanceWindow) (bool, error)

	DeleteHosts(ctx context.Context, clusterID string, FQDNs []string, revision int64) ([]hosts.Host, error)
	ModifyHost(ctx context.Context, args models.ModifyHostArgs) error
	ModifyHostPublicIP(ctx context.Context, clusterID string, fqdn string, revision int64, assignPublicIP bool) error

	UpdateHostPillar(ctx context.Context, cid, fqdn string, revision int64, pillar pillars.Marshaler) error
	UpdatePillar(ctx context.Context, cid string, revision int64, pillar Pillar) error
	UpdateSubClusterPillar(ctx context.Context, cid, subcid string, revision int64, pillar pillars.Marshaler) error
	UpdateShardPillar(ctx context.Context, cid, shardid string, revision int64, pillar pillars.Marshaler) error
	DeleteSubCluster(ctx context.Context, cid, subcid string, revision int64) error

	DeleteShard(ctx context.Context, cid, shardID string, revision int64) error
	RescheduleMaintenance(ctx context.Context, cid string, rescheduleType clustermodels.RescheduleType, maintenanceTime optional.Time) (time.Time, error)
}

type baseModifier interface {
	Validator

	CreateSubCluster(ctx context.Context, args models.CreateSubClusterArgs) (clustermodels.SubCluster, error)
	CreateKubernetesSubCluster(ctx context.Context, args models.CreateSubClusterArgs) (clustermodels.SubCluster, error)
	CreateShard(ctx context.Context, args models.CreateShardArgs) (clustermodels.Shard, error)

	AddHosts(ctx context.Context, args []models.AddHostArgs) ([]hosts.Host, error)

	AddSubClusterPillar(ctx context.Context, cid, subcid string, revision int64, pillar pillars.Marshaler) error
	AddShardPillar(ctx context.Context, cid, shardid string, revision int64, pillar pillars.Marshaler) error
	AddHostPillar(ctx context.Context, cid, fqdn string, revision int64, pillar pillars.Marshaler) error

	GenerateFQDN(geoName string, vtype environment.VType, platform compute.Platform) (string, error)
	GenerateSemanticFQDNs(cloudType environment.CloudType, clusterType clustermodels.Type, zonesToCreate ZoneHostsList,
		zonesCurrent ZoneHostsList, shardName string, cid string, vtype environment.VType, platform compute.Platform) (map[string][]string, error)

	// TODO move somewhere else
	RegionByName(ctx context.Context, region string) (environment.Region, error)
	ListAvailableZones(ctx context.Context, session sessions.Session, forceFilterDecommissionedZone bool) ([]environment.Zone, error)
	ListAvailableZonesForCloudAndRegion(ctx context.Context, session sessions.Session, cloudType environment.CloudType,
		regionID string, forceFilterDecommissionedZone bool) ([]string, error)
	SelectZonesForCloudAndRegion(ctx context.Context, session sessions.Session,
		cloudType environment.CloudType, regionID string, forceFilterDecommissionedZone bool, zoneCount int) ([]string, error)
	ResourcePresetByCPU(ctx context.Context, clusterType clustermodels.Type, role hosts.Role, flavorType optional.String,
		generation optional.Int64, minCPU float64, zones, featureFlags []string) (resources.DefaultPreset, error)
	ResourcePresetFromDefaultConfig(clusterType clustermodels.Type, role hosts.Role) (resources.DefaultPreset, error)

	AddDiskPlacementGroup(ctx context.Context, args models.AddDiskPlacementGroupArgs) (int64, error)
	AddDisk(ctx context.Context, args models.AddDiskArgs) (int64, error)

	AddPlacementGroup(ctx context.Context, args models.AddPlacementGroupArgs) (int64, error)
}

type Validator interface {
	ValidateResources(ctx context.Context, session sessions.Session, typ clustermodels.Type, hostGroups ...HostGroup) (ResolvedHostGroups, bool, error)
}
