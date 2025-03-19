package mysql

import (
	"context"
	"sort"

	mysqlv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/mysql/v1"
	mysqlv1console "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/mysql/v1/console"
	"a.yandex-team.ru/cloud/mdb/internal/compute/compute"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	grpcapi "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/api/grpc/common"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/console"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/mysql"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	consolemodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/console"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts"
	"a.yandex-team.ru/library/go/slices"
)

// ClusterService implements DB-specific gRPC methods
type ConsoleClusterService struct {
	mysqlv1console.UnimplementedClusterServiceServer

	Console       console.Console
	mysql         mysql.MySQL
	saltEnvMapper grpcapi.SaltEnvMapper
}

func NewConsoleClusterService(cnsl console.Console, mysql mysql.MySQL, saltEnvsCfg logic.SaltEnvsConfig) *ConsoleClusterService {
	return &ConsoleClusterService{
		Console: cnsl,
		mysql:   mysql,
		saltEnvMapper: grpcapi.NewSaltEnvMapper(
			int64(mysqlv1.Cluster_PRODUCTION),
			int64(mysqlv1.Cluster_PRESTABLE),
			int64(mysqlv1.Cluster_ENVIRONMENT_UNSPECIFIED),
			saltEnvsCfg),
	}
}

func (ccs *ConsoleClusterService) Get(ctx context.Context, cfgReq *mysqlv1console.GetMysqlClustersConfigRequest) (*mysqlv1console.MysqlClustersConfig, error) {
	resourcePresets, err := ccs.Console.GetResourcePresetsByClusterType(ctx, clusters.TypeMySQL, cfgReq.FolderId, true)
	if err != nil {
		return nil, err
	}
	hostGroupHostType, err := ccs.mysql.GetHostGroupType(ctx, slices.DedupStrings(cfgReq.GetHostGroupIds()))
	if err != nil {
		return nil, err
	}

	clusterVersions, err := ccs.mysql.GetDefaultVersions(ctx)
	if err != nil {
		return nil, err
	}

	resourcePresetsGRPC, err := ccs.resourcePresetsToGRPC(resourcePresets, hostGroupHostType)
	if err != nil {
		return nil, err
	}

	roleDefaultResources, err := ccs.Console.GetDefaultResourcesByClusterType(clusters.TypeMySQL, hosts.RoleMySQL)
	if err != nil {
		return nil, err
	}

	return &mysqlv1console.MysqlClustersConfig{
		ClusterName: &mysqlv1console.NameValidator{
			Regexp: models.DefaultClusterNamePattern,
			Min:    models.DefaultClusterNameMinLen,
			Max:    models.DefaultClusterNameMaxLen,
		},
		DbName: &mysqlv1console.NameValidator{
			Regexp:    models.DefaultDatabaseNamePattern,
			Min:       models.DefaultDatabaseNameMinLen,
			Max:       models.DefaultDatabaseNameMaxLen,
			Blacklist: []string{"mysql", "sys", "information_schema", "performance_schema"},
		},
		UserName: &mysqlv1console.NameValidator{
			Regexp:    models.DefaultUserNamePattern,
			Min:       models.DefaultUserNameMinLen,
			Max:       models.DefaultUserNameMaxLen,
			Blacklist: []string{"admin", "monitor", "root", "mdb_admin", "repl", "mysql.session", "mysql.sys"},
		},
		Password: &mysqlv1console.NameValidator{
			Regexp: "[^\000'\"\b\n\r\t\x1A\\%]*",
			Min:    models.DefaultUserPasswordMinLen,
			Max:    models.DefaultUserPasswordMaxLen,
		},

		HostCountLimits: &mysqlv1console.MysqlClustersConfig_HostCountLimits{
			MinHostCount:         1,
			MaxHostCount:         7,
			HostCountPerDiskType: []*mysqlv1console.MysqlClustersConfig_HostCountLimits_HostCountPerDiskType{},
		},

		Versions:          common.CollectAllVersionNames(clusterVersions),
		DefaultVersion:    common.CollectDefaultVersion(clusterVersions),
		AvailableVersions: ccs.availableVersionsToGRPC(clusterVersions),

		ResourcePresets:  resourcePresetsGRPC,
		DefaultResources: ccs.defaultResourcesToGRPC(roleDefaultResources),
	}, nil
}

func (ccs *ConsoleClusterService) availableVersionsToGRPC(clusterVersions []consolemodels.DefaultVersion) []*mysqlv1console.MysqlClustersConfig_VersionInfo {
	var availableVersions []*mysqlv1console.MysqlClustersConfig_VersionInfo
	for _, ver := range clusterVersions {
		if ver.IsDeprecated {
			continue
		}
		// FIXME: should we support `MDB_MYSQL_5_7_TO_8_0_UPGRADE` feature flag?
		availableVersions = append(availableVersions, ccs.versionsToGRPC(ver))
	}
	sort.SliceStable(availableVersions, func(i, j int) bool {
		return availableVersions[i].Name < availableVersions[j].Name
	})
	return availableVersions
}

func (ccs *ConsoleClusterService) versionsToGRPC(ver consolemodels.DefaultVersion) *mysqlv1console.MysqlClustersConfig_VersionInfo {
	// There is a chance that `UpdatableTo` won't work: MDB-13009
	return &mysqlv1console.MysqlClustersConfig_VersionInfo{
		Id:          ver.Name,
		Name:        ver.Name,
		Deprecated:  ver.IsDeprecated,
		UpdatableTo: ver.UpdatableTo,
	}
}

func (ccs *ConsoleClusterService) resourcePresetsToGRPC(resourcePresets []consolemodels.ResourcePreset, hostGroupHostType map[string]compute.HostGroupHostType) ([]*mysqlv1console.MysqlClustersConfig_ResourcePreset, error) {
	aggregated := common.AggregateResourcePresets(resourcePresets)
	hostRoleAgg, ok := aggregated[hosts.RoleMySQL]
	if !ok {
		return nil, semerr.Internalf("No resource preset found for role %v", hosts.RoleMySQL)
	}

	hostGroupGenerations := make(map[int64]bool)
	for _, hght := range hostGroupHostType {
		generation := hght.Generation()
		hostGroupGenerations[generation] = true
	}

	var lastRecord *consolemodels.ResourcePreset
	var minCoresInHostGroups int64
	var minMemoryInHostGroups int64

	resourcesGRPC := make([]*mysqlv1console.MysqlClustersConfig_ResourcePreset, 0)
	for _, presetAgg := range hostRoleAgg {
		zonesGRPC := make([]*mysqlv1console.MysqlClustersConfig_ResourcePreset_Zone, 0)
		for _, zoneAgg := range presetAgg {
			diskTypesGRPC := make([]*mysqlv1console.MysqlClustersConfig_ResourcePreset_Zone_DiskType, 0)
			for _, record := range zoneAgg {

				diskTypeGRPC := &mysqlv1console.MysqlClustersConfig_ResourcePreset_Zone_DiskType{
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
					diskTypeGRPC.DiskSize = &mysqlv1console.MysqlClustersConfig_ResourcePreset_Zone_DiskType_DiskSizes_{
						DiskSizes: &mysqlv1console.MysqlClustersConfig_ResourcePreset_Zone_DiskType_DiskSizes{
							Sizes: diskSizes,
						},
					}
				} else if len(record.DiskSizes) == 0 {
					diskTypeGRPC.DiskSize = &mysqlv1console.MysqlClustersConfig_ResourcePreset_Zone_DiskType_DiskSizeRange_{
						DiskSizeRange: &mysqlv1console.MysqlClustersConfig_ResourcePreset_Zone_DiskType_DiskSizeRange{
							Min: record.DiskSizeRange.Lower,
							Max: record.DiskSizeRange.Upper,
						},
					}
				} else {
					diskTypeGRPC.DiskSize = &mysqlv1console.MysqlClustersConfig_ResourcePreset_Zone_DiskType_DiskSizes_{
						DiskSizes: &mysqlv1console.MysqlClustersConfig_ResourcePreset_Zone_DiskType_DiskSizes{
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
			zoneGRPC := mysqlv1console.MysqlClustersConfig_ResourcePreset_Zone{
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
		presetGRPC := &mysqlv1console.MysqlClustersConfig_ResourcePreset{
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

func (ccs *ConsoleClusterService) defaultResourcesToGRPC(roleDefaultResources consolemodels.DefaultResources) *mysqlv1console.MysqlClustersConfig_DefaultResources {
	return &mysqlv1console.MysqlClustersConfig_DefaultResources{
		ResourcePresetId: roleDefaultResources.ResourcePresetExtID,
		DiskTypeId:       roleDefaultResources.DiskTypeExtID,
		DiskSize:         roleDefaultResources.DiskSize,
		Generation:       roleDefaultResources.Generation,
		GenerationName:   roleDefaultResources.GenerationName,
	}
}
