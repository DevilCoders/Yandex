package provider

import (
	"context"
	"sort"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	clustersprovider "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/clusters/provider"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/sessions"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	consolemodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/console"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/pillars"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (c *Console) ListClusters(ctx context.Context, projectID string, pageSize optional.Int64, pageToken clusters.ClusterPageToken) ([]consolemodels.Cluster, error) {
	ctx, session, err := c.sessions.Begin(ctx, sessions.ResolveByFolder(projectID, models.PermMDBAllRead), sessions.WithPreferStandby())
	if err != nil {
		return nil, err
	}
	defer func() { c.sessions.Rollback(ctx) }()

	cls, err := c.metaDB.Clusters(ctx, models.ListClusterArgs{
		FolderID:      session.FolderCoords.FolderID,
		Visibility:    models.VisibilityVisible,
		Limit:         pageSize,
		PageTokenName: optional.NewString(pageToken.LastClusterName),
	})
	if err != nil {
		return nil, err
	}

	res := make([]consolemodels.Cluster, 0, len(cls))
	for _, cl := range cls {
		health, _ := c.health.Cluster(ctx, cl.ClusterID)
		cs, ok := c.clusterSpecific[cl.Type]
		if !ok {
			return nil, xerrors.Errorf("invalid cluster type %q", cl.Type.String())
		}

		version, err := cs.Version(ctx, c.metaDB, cl.ClusterID)
		if err != nil {
			return nil, err
		}

		var pillar pillars.ClusterPillar
		if err := pillar.UnmarshalPillar(cl.Pillar); err != nil {
			return nil, err
		}

		hosts, _, _, err := c.metaDB.ListHosts(ctx, cl.ClusterID, 0, optional.Int64{})
		if err != nil {
			return nil, err
		}

		resources := map[string]consolemodels.ClusterResources{}

		for _, host := range hosts {
			hostRole := host.Roles[0].Stringified()
			resource, found := resources[hostRole]
			if !found {
				resources[hostRole] = consolemodels.ClusterResources{
					HostType:            host.Roles[0],
					HostCount:           1,
					ResourcePresetExtID: host.ResourcePresetExtID,
					DiskSize:            host.SpaceLimit,
				}
			} else {
				resource.HostCount += 1
				resources[hostRole] = resource
			}
		}

		var allClusterResources []consolemodels.ClusterResources
		for _, clusterResources := range resources {
			allClusterResources = append(allClusterResources, clusterResources)
		}

		res = append(res, consolemodels.Cluster{
			ID:          cl.ClusterID,
			ProjectID:   projectID,
			CloudType:   pillar.Data.CloudType,
			RegionID:    pillar.Data.RegionID,
			CreateTime:  cl.CreatedAt,
			Name:        cl.Name,
			Description: cl.Description,
			Status:      cl.Status,
			Health:      health,
			Type:        cl.Type,
			Version:     version,
			Resources:   allClusterResources,
		})
	}

	return res, nil
}

func (c *Console) ProjectByClusterID(ctx context.Context, clusterID string) (string, error) {
	ctx, session, err := c.sessions.Begin(ctx, sessions.ResolveByCluster(clusterID, models.PermMDBAllRead), sessions.WithPreferStandby())
	if err != nil {
		return "", err
	}
	defer func() { c.sessions.Rollback(ctx) }()

	return session.FolderCoords.FolderExtID, nil
}

func (c *Console) GetDiskIOLimit(ctx context.Context, spaceLimit int64, diskType string, resourcePreset string) (int64, error) {
	return c.metaDB.DiskIOLimit(ctx, spaceLimit, diskType, resourcePreset)
}

func (c *Console) GetClusterConfig(ctx context.Context, folderID string, clusterType clusters.Type) (consolemodels.ClustersConfig, error) {
	featureFlags, err := c.GetFeatureFlags(ctx, folderID)
	if err != nil {
		return consolemodels.ClustersConfig{}, err
	}

	var withDecomissioned bool
	if featureFlags.Has(clustersprovider.FeatureFlagAllowDecommissionedResourcePreset) {
		withDecomissioned = true
	}

	resourcePresets, err := c.GetResourcePresetsByClusterType(ctx, clusterType, folderID, withDecomissioned)
	if err != nil {
		return consolemodels.ClustersConfig{}, err
	}

	defaultResourcesForRoles := map[hosts.Role]consolemodels.DefaultResources{}
	for _, role := range clusterType.Roles().Possible {
		defaultResources, err := c.GetDefaultResourcesByClusterType(clusterType, role)
		if err != nil {
			return consolemodels.ClustersConfig{}, err
		}

		defaultResourcesForRoles[role] = defaultResources
	}

	hostTypes := getHostTypes(resourcePresets)
	for i, hostType := range hostTypes {
		defaultResources, found := defaultResourcesForRoles[hostType.Type]
		if !found {
			return consolemodels.ClustersConfig{}, xerrors.Errorf("default resources for %s cluster host with role %s not found.", clusterType.String(), hostType.Type.String())
		}
		hostTypes[i].DefaultResources = defaultResources
	}

	return consolemodels.ClustersConfig{
		ClusterNameValidator:  consolemodels.ClusterNameValidator,
		DatabaseNameValidator: consolemodels.DatabaseNameValidator,
		UserNameValidator:     consolemodels.UserNameValidator,
		PasswordValidator:     consolemodels.PasswordValidator,
		HostCountLimits:       getHostCountLimits(resourcePresets),
		HostTypes:             hostTypes,
	}, nil
}

func getHostCountLimits(presets []consolemodels.ResourcePreset) consolemodels.HostCountLimits {
	minHostCount, maxHostCount := consolemodels.MinMaxHosts(presets, optional.String{})
	diskTypes := consolemodels.DiskTypes(presets)

	hostCountPerDiskType := map[string]int64{}

	for _, diskType := range diskTypes {
		minHostsPerDiskType, _ := consolemodels.MinMaxHosts(presets, optional.NewString(diskType))
		if minHostsPerDiskType > minHostCount {
			hostCountPerDiskType[diskType] = minHostsPerDiskType
		}
	}

	var hostCounts []consolemodels.DiskTypeHostCount
	for diskType, hostCount := range hostCountPerDiskType {
		hostCounts = append(hostCounts, consolemodels.DiskTypeHostCount{
			DiskTypeID:   diskType,
			MinHostCount: hostCount,
		})
	}

	return consolemodels.HostCountLimits{
		MinHostCount:         minHostCount,
		MaxHostCount:         maxHostCount,
		HostCountPerDiskType: hostCounts,
	}
}

func getHostTypes(presets []consolemodels.ResourcePreset) []consolemodels.HostType {
	var res []consolemodels.HostType

	presetsByRole := consolemodels.GroupResourcePresetsByRole(presets)
	for role, rolePresets := range presetsByRole {
		var resourcePresetsForRole []consolemodels.HostTypeResourcePreset

		presetsByExtID := consolemodels.GroupResourcePresetsByExtID(rolePresets)
		for extID, extIDPresets := range presetsByExtID {
			var zones []consolemodels.HostTypeZone

			presetsByZone := consolemodels.GroupResourcePresetsByZone(extIDPresets)
			for zone, zonePresets := range presetsByZone {
				var diskTypes []consolemodels.HostTypeDiskType

				presetsByDiskType := consolemodels.GroupResourcePresetsByDiskType(zonePresets)
				for diskTypeID, diskTypePresets := range presetsByDiskType {
					for _, rp := range diskTypePresets {
						diskType := consolemodels.HostTypeDiskType{
							ID:       diskTypeID,
							MinHosts: rp.MinHosts,
							MaxHosts: rp.MaxHosts,
						}

						if rp.DiskSizeRange.Upper > 0 {
							diskTypes = append(diskTypes, diskType.WithSizeRange(&consolemodels.Int64Range{
								Lower: rp.DiskSizeRange.Lower,
								Upper: rp.DiskSizeRange.Upper,
							}))
						}

						if len(rp.DiskSizes) > 0 {
							diskTypes = append(diskTypes, diskType.WithSizes(rp.DiskSizes))
						}
					}
				}

				sort.Slice(diskTypes, func(i, j int) bool {
					return diskTypes[i].ID < diskTypes[j].ID
				})

				zones = append(zones, consolemodels.HostTypeZone{
					ID:        zone,
					DiskTypes: diskTypes,
				})
			}

			sort.Slice(zones, func(i, j int) bool {
				return zones[i].ID < zones[j].ID
			})

			resourcePresetsForRole = append(resourcePresetsForRole, consolemodels.HostTypeResourcePreset{
				ID:             extID,
				CPULimit:       extIDPresets[0].CPULimit,
				RAMLimit:       extIDPresets[0].MemoryLimit,
				Generation:     extIDPresets[0].Generation,
				GenerationName: extIDPresets[0].GenerationName,
				FlavorType:     extIDPresets[0].FlavorType,
				CPUFraction:    extIDPresets[0].CPUFraction,
				Decomissioning: extIDPresets[0].Decommissioned,
				Zones:          zones,
			})
		}

		sort.Slice(resourcePresetsForRole, func(i, j int) bool {
			return resourcePresetsForRole[i].ID < resourcePresetsForRole[j].ID
		})

		res = append(res, consolemodels.HostType{
			Type:            role,
			ResourcePresets: resourcePresetsForRole,
		})
	}

	sort.Slice(res, func(i, j int) bool {
		return res[i].Type < res[j].Type
	})

	return res
}
