package console

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/internal/featureflags"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	consolemodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/console"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/environment"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/resources"
)

//go:generate ../../../../scripts/mockgen.sh Console

// Console is committed use discount
type Console interface {
	GetPlatforms(ctx context.Context) ([]consolemodels.Platform, error)
	GetUsedResources(ctx context.Context, clouds []string) ([]consolemodels.UsedResources, error)
	GetResourcePresetsByClusterType(ctx context.Context, clusterType clusters.Type, folderID string, withDecomissioned bool) ([]consolemodels.ResourcePreset, error)
	GetDefaultResourcesByClusterType(clusterType clusters.Type, role hosts.Role) (consolemodels.DefaultResources, error)
	GetConnectionDomain(ctx context.Context) (string, error)

	GetCloudsByClusterType(ctx context.Context, clusterType clusters.Type, pageSize, offset int64) ([]consolemodels.Cloud, int64, error)
	GetResourcePresetsByCloudRegion(ctx context.Context, cloudType environment.CloudType, region string, clusterType clusters.Type, projectID string) ([]consolemodels.ResourcePreset, error)

	GetClusterConfig(ctx context.Context, folderID string, clusterType clusters.Type) (consolemodels.ClustersConfig, error)

	FolderStats(ctx context.Context, folderID string) (consolemodels.FolderStats, error)

	InitResourcesIfEmpty(resources *models.ClusterResources, role hosts.Role, clusterType clusters.Type) error

	GetDefaultVersions(ctx context.Context, clusterType clusters.Type, env environment.SaltEnv, component string) ([]consolemodels.DefaultVersion, error)

	ResourcePresetByExtID(ctx context.Context, extID string) (resources.Preset, error)

	GetFeatureFlags(ctx context.Context, folderID string) (featureflags.FeatureFlags, error)

	ListClusters(ctx context.Context, projectID string, pageSize optional.Int64, pageToken clusters.ClusterPageToken) ([]consolemodels.Cluster, error)

	ProjectByClusterID(ctx context.Context, clusterID string) (string, error)

	GetNetworksByCloudExtID(ctx context.Context, cloudExtID string) ([]string, error)

	GetDiskIOLimit(ctx context.Context, spaceLimit int64, diskType string, resourcePreset string) (int64, error)
}

type ClusterSpecific interface {
	Version(ctx context.Context, metadb metadb.Backend, cid string) (string, error)
}
