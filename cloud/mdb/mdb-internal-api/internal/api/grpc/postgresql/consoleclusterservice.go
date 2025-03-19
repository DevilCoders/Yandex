package postgresql

import (
	"context"
	"sort"

	pgv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/postgresql/v1"
	pgv1console "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/postgresql/v1/console"
	"a.yandex-team.ru/cloud/mdb/internal/compute/compute"
	sessions "a.yandex-team.ru/cloud/mdb/internal/featureflags"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	grpcapi "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc/common"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/console"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/postgresql"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/postgresql/pgmodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	consolemodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/console"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts"
	"a.yandex-team.ru/library/go/slices"
)

// ClusterService implements DB-specific gRPC methods
type ConsoleClusterService struct {
	pgv1console.UnimplementedClusterServiceServer

	Console       console.Console
	pg            postgresql.PostgreSQL
	saltEnvMapper grpcapi.SaltEnvMapper
}

func NewConsoleClusterService(cnsl console.Console, pg postgresql.PostgreSQL, saltEnvsCfg logic.SaltEnvsConfig) *ConsoleClusterService {
	return &ConsoleClusterService{
		Console: cnsl,
		pg:      pg,
		saltEnvMapper: grpcapi.NewSaltEnvMapper(
			int64(pgv1.Cluster_PRODUCTION),
			int64(pgv1.Cluster_PRESTABLE),
			int64(pgv1.Cluster_ENVIRONMENT_UNSPECIFIED),
			saltEnvsCfg),
	}
}

func (ccs *ConsoleClusterService) Get(ctx context.Context, cfgReq *pgv1console.GetPostgresqlClustersConfigRequest) (*pgv1console.PostgresqlClustersConfig, error) {
	featureFlags, err := ccs.Console.GetFeatureFlags(ctx, cfgReq.FolderId)
	if err != nil {
		return nil, err
	}

	resourcePresets, err := ccs.Console.GetResourcePresetsByClusterType(ctx, clusters.TypePostgreSQL, cfgReq.FolderId, true)
	if err != nil {
		return nil, err
	}
	hostGroupHostType, err := ccs.pg.GetHostGroupType(ctx, slices.DedupStrings(cfgReq.GetHostGroupIds()))
	if err != nil {
		return nil, err
	}

	clusterVersions, err := ccs.pg.GetDefaultVersions(ctx)
	if err != nil {
		return nil, err
	}

	resourcePresetsGRPC, err := ccs.resourcePresetsToGRPC(resourcePresets, hostGroupHostType)
	if err != nil {
		return nil, err
	}

	roleDefaultResources, err := ccs.Console.GetDefaultResourcesByClusterType(clusters.TypePostgreSQL, hosts.RolePostgreSQL)
	if err != nil {
		return nil, err
	}

	return &pgv1console.PostgresqlClustersConfig{
		ClusterName: &pgv1console.NameValidator{
			Regexp: models.DefaultClusterNamePattern,
			Min:    models.DefaultClusterNameMinLen,
			Max:    models.DefaultClusterNameMaxLen,
		},
		DbName: &pgv1console.NameValidator{
			Regexp:    models.DefaultDatabaseNamePattern,
			Min:       models.DefaultDatabaseNameMinLen,
			Max:       models.DefaultDatabaseNameMaxLen,
			Blacklist: []string{"template0", "template1", "postgres"},
		},
		UserName: &pgv1console.NameValidator{
			Regexp:    "^(?!pg_)[a-zA-Z0-9_][a-zA-Z0-9_-]*$",
			Min:       models.DefaultUserNameMinLen,
			Max:       63,
			Blacklist: []string{"admin", "repl", "monitor", "postgres", "public", "none", "mdb_admin", "mdb_replication"},
		},
		Password: &pgv1console.NameValidator{
			Regexp: models.DefaultUserPasswordPattern,
			Min:    models.DefaultUserPasswordMinLen,
			Max:    models.DefaultUserPasswordMaxLen,
		},

		HostCountLimits: &pgv1console.PostgresqlClustersConfig_HostCountLimits{
			MinHostCount:         1,
			MaxHostCount:         7,
			HostCountPerDiskType: []*pgv1console.PostgresqlClustersConfig_HostCountLimits_HostCountPerDiskType{},
		},

		Versions:          ccs.collectAllVersionNames(clusterVersions, featureFlags),
		DefaultVersion:    common.CollectDefaultVersion(clusterVersions),
		AvailableVersions: ccs.availableVersionsToGRPC(clusterVersions, featureFlags),

		ResourcePresets:  resourcePresetsGRPC,
		DefaultResources: ccs.defaultResourcesToGRPC(roleDefaultResources),
	}, nil
}

func (ccs *ConsoleClusterService) collectAllVersionNames(clusterVersions []consolemodels.DefaultVersion, featureFlags sessions.FeatureFlags) []string {
	var versions []string
	for _, ver := range clusterVersions {
		if ver.IsDeprecated {
			continue
		}
		ff, ok := pgmodels.PostgreSQLVersionsFeatureFlags[ver.Name]
		if ok && !featureFlags.Has(ff) {
			continue
		}
		versions = append(versions, ver.Name)
	}

	sort.Strings(versions)
	return versions
}

func (ccs *ConsoleClusterService) availableVersionsToGRPC(clusterVersions []consolemodels.DefaultVersion, featureFlags sessions.FeatureFlags) []*pgv1console.PostgresqlClustersConfig_VersionInfo {
	var availableVersions []*pgv1console.PostgresqlClustersConfig_VersionInfo
	for _, ver := range clusterVersions {
		if ver.IsDeprecated {
			continue
		}
		ff, ok := pgmodels.PostgreSQLVersionsFeatureFlags[ver.Name]
		if ok && !featureFlags.Has(ff) {
			continue
		}
		availableVersions = append(availableVersions, ccs.versionsToGRPC(ver))
	}
	sort.SliceStable(availableVersions, func(i, j int) bool {
		return availableVersions[i].Name < availableVersions[j].Name
	})
	return availableVersions
}

func (ccs *ConsoleClusterService) versionsToGRPC(ver consolemodels.DefaultVersion) *pgv1console.PostgresqlClustersConfig_VersionInfo {
	return &pgv1console.PostgresqlClustersConfig_VersionInfo{
		Id:                 ver.Name,
		Name:               ver.Name,
		Deprecated:         ver.IsDeprecated,
		UpdatableTo:        ver.UpdatableTo,
		VersionedConfigKey: pgmodels.PostgreSQLVersionedKeys[ver.Name],
	}
}

func (ccs *ConsoleClusterService) resourcePresetsToGRPC(resourcePresets []consolemodels.ResourcePreset, hostGroupHostType map[string]compute.HostGroupHostType) ([]*pgv1console.PostgresqlClustersConfig_ResourcePreset, error) {
	aggregated := common.AggregateResourcePresets(resourcePresets)
	hostRoleAgg, ok := aggregated[hosts.RolePostgreSQL]
	if !ok {
		return nil, semerr.Internalf("No resource preset found for role %v", hosts.RolePostgreSQL)
	}

	hostGroupGenerations := make(map[int64]bool)
	for _, hght := range hostGroupHostType {
		generation := hght.Generation()
		hostGroupGenerations[generation] = true
	}

	var lastRecord *consolemodels.ResourcePreset
	var minCoresInHostGroups int64
	var minMemoryInHostGroups int64

	resourcesGRPC := make([]*pgv1console.PostgresqlClustersConfig_ResourcePreset, 0)
	for _, presetAgg := range hostRoleAgg {
		zonesGRPC := make([]*pgv1console.PostgresqlClustersConfig_ResourcePreset_Zone, 0)
		for _, zoneAgg := range presetAgg {
			diskTypesGRPC := make([]*pgv1console.PostgresqlClustersConfig_ResourcePreset_Zone_DiskType, 0)
			for _, record := range zoneAgg {

				diskTypeGRPC := &pgv1console.PostgresqlClustersConfig_ResourcePreset_Zone_DiskType{
					DiskTypeId: record.DiskTypeExtID,
					MaxHosts:   record.MaxHosts,
					MinHosts:   record.MinHosts,
				}
				// Providing non empty disk size field
				_, _, minDisks, diskSize, _ := compute.GetMinHostGroupResources(hostGroupHostType, record.Zone)
				if record.DiskTypeExtID == "local-ssd" && len(hostGroupHostType) > 0 && diskSize > 0 {
					// Fill local-ssd sizes according to host group
					var diskSizes []int64
					for i := diskSize; i <= minDisks*diskSize; i = i + diskSize {
						diskSizes = append(diskSizes, i)
					}
					diskTypeGRPC.DiskSize = &pgv1console.PostgresqlClustersConfig_ResourcePreset_Zone_DiskType_DiskSizes_{
						DiskSizes: &pgv1console.PostgresqlClustersConfig_ResourcePreset_Zone_DiskType_DiskSizes{
							Sizes: diskSizes,
						},
					}
				} else if len(record.DiskSizes) == 0 {
					diskTypeGRPC.DiskSize = &pgv1console.PostgresqlClustersConfig_ResourcePreset_Zone_DiskType_DiskSizeRange_{
						DiskSizeRange: &pgv1console.PostgresqlClustersConfig_ResourcePreset_Zone_DiskType_DiskSizeRange{
							Min: record.DiskSizeRange.Lower,
							Max: record.DiskSizeRange.Upper,
						},
					}
				} else {
					diskTypeGRPC.DiskSize = &pgv1console.PostgresqlClustersConfig_ResourcePreset_Zone_DiskType_DiskSizes_{
						DiskSizes: &pgv1console.PostgresqlClustersConfig_ResourcePreset_Zone_DiskType_DiskSizes{
							Sizes: record.DiskSizes,
						},
					}
				}
				diskTypesGRPC = append(diskTypesGRPC, diskTypeGRPC)
				lastRecord = &record
			}
			// Filter zones according host group
			if len(hostGroupHostType) > 0 {
				minCores, minMemory, _, _, _ := compute.GetMinHostGroupResources(hostGroupHostType, lastRecord.Zone)
				if minCores == 0 {
					continue
				}
				if minCores > minCoresInHostGroups {
					minCoresInHostGroups = minCores
				}
				if minMemory > minMemoryInHostGroups {
					minMemoryInHostGroups = minMemory
				}
			}
			// Using last record from this zone to fill zone info
			zoneGRPC := pgv1console.PostgresqlClustersConfig_ResourcePreset_Zone{
				ZoneId:    lastRecord.Zone,
				DiskTypes: diskTypesGRPC,
			}
			zonesGRPC = append(zonesGRPC, &zoneGRPC)
		}
		sort.Slice(zonesGRPC, func(i, j int) bool {
			zoneA := zonesGRPC[i]
			zoneB := zonesGRPC[j]
			return zoneA.ZoneId < zoneB.ZoneId
		})
		// Filter preset info according host group
		if len(hostGroupHostType) > 0 {
			if lastRecord.CPULimit > minCoresInHostGroups {
				continue
			}
			if lastRecord.MemoryLimit > minMemoryInHostGroups {
				continue
			}
		}

		_, equalGenerations := hostGroupGenerations[lastRecord.Generation]
		// If there is more than one generation in host groups
		// or just one, but not equal to current flavor generation
		// then skip this flavor
		if (len(hostGroupGenerations) > 1) ||
			(len(hostGroupGenerations) == 1 && !equalGenerations) {
			continue
		}

		// Using last record to fill preset info
		presetGRPC := &pgv1console.PostgresqlClustersConfig_ResourcePreset{
			PresetId:       lastRecord.ExtID,
			CpuLimit:       lastRecord.CPULimit,
			MemoryLimit:    lastRecord.MemoryLimit,
			Generation:     lastRecord.Generation,
			GenerationName: lastRecord.GenerationName,
			Type:           lastRecord.FlavorType,
			CpuFraction:    lastRecord.CPUFraction,
			Decomissioning: lastRecord.Decommissioned,
			Zones:          zonesGRPC,
		}
		resourcesGRPC = append(resourcesGRPC, presetGRPC)

	}
	sort.Slice(resourcesGRPC, func(i, j int) bool {
		if resourcesGRPC[i].Generation != resourcesGRPC[j].Generation {
			return resourcesGRPC[i].Generation < resourcesGRPC[j].Generation
		}
		if resourcesGRPC[i].CpuLimit != resourcesGRPC[j].CpuLimit {
			return resourcesGRPC[i].CpuLimit < resourcesGRPC[j].CpuLimit
		}
		if resourcesGRPC[i].MemoryLimit != resourcesGRPC[j].MemoryLimit {
			return resourcesGRPC[i].MemoryLimit < resourcesGRPC[j].MemoryLimit
		}
		return false
	})

	return resourcesGRPC, nil
}

func (ccs *ConsoleClusterService) defaultResourcesToGRPC(roleDefaultResources consolemodels.DefaultResources) *pgv1console.PostgresqlClustersConfig_DefaultResources {
	return &pgv1console.PostgresqlClustersConfig_DefaultResources{
		ResourcePresetId: roleDefaultResources.ResourcePresetExtID,
		DiskTypeId:       roleDefaultResources.DiskTypeExtID,
		DiskSize:         roleDefaultResources.DiskSize,
		Generation:       roleDefaultResources.Generation,
		GenerationName:   roleDefaultResources.GenerationName,
	}
}
