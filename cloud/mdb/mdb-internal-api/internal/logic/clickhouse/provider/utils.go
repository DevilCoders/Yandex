package provider

import (
	"context"
	"fmt"
	"math/rand"
	"sort"
	"strings"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/compute"
	networkProvider "a.yandex-team.ru/cloud/mdb/internal/network"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/clickhouse/chmodels"
	validators "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/clickhouse/provider/internal"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/clickhouse/provider/internal/chpillars"
	clusterslogic "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/sessions"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/environment"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/pillars"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/resources"
	"a.yandex-team.ru/library/go/slices"
)

type creatorModifier interface {
	AddHosts(ctx context.Context, args []models.AddHostArgs) ([]hosts.Host, error)
	GenerateSemanticFQDNs(cloudType environment.CloudType, clusterType clusters.Type, zonesToCreate clusterslogic.ZoneHostsList,
		zonesCurrent clusterslogic.ZoneHostsList, shardName string, cid string, vtype environment.VType, platform compute.Platform) (map[string][]string, error)
	CreateSubCluster(ctx context.Context, args models.CreateSubClusterArgs) (clusters.SubCluster, error)
	AddSubClusterPillar(ctx context.Context, cid, subcid string, revision int64, pillar pillars.Marshaler) error
	CreateShard(ctx context.Context, args models.CreateShardArgs) (clusters.Shard, error)
	AddShardPillar(ctx context.Context, cid, shardid string, revision int64, pillar pillars.Marshaler) error
	ListAvailableZones(ctx context.Context, session sessions.Session, forceFilterDecommissionedZone bool) ([]environment.Zone, error)
	ResourcePresetByCPU(ctx context.Context, clusterType clusters.Type, role hosts.Role, flavorType optional.String,
		generation optional.Int64, minCPU float64, zones, featureFlags []string) (resources.DefaultPreset, error)
}
type prepareModifier interface {
	ResourcePresetFromDefaultConfig(clusterType clusters.Type, role hosts.Role) (resources.DefaultPreset, error)
	ValidateResources(ctx context.Context, session sessions.Session, typ clusters.Type, hostGroups ...clusterslogic.HostGroup) (clusterslogic.ResolvedHostGroups, bool, error)
}

func (ch *ClickHouse) chCluster(ctx context.Context, reader clusterslogic.Reader, cid string) (cluster, error) {
	var pillar chpillars.ClusterCH
	c, err := reader.ClusterByClusterID(ctx, cid, clusters.TypeClickHouse, models.VisibilityVisible)
	if err != nil {
		return cluster{}, err
	}

	if err := c.Pillar(&pillar); err != nil {
		return cluster{}, err
	}

	return cluster{
		Cluster: c.Cluster,
		Pillar:  &pillar,
	}, nil
}

func (ch *ClickHouse) chSubCluster(ctx context.Context, reader clusterslogic.Reader, cid string) (subCluster, error) {
	var pillar chpillars.SubClusterCH
	sc, err := reader.SubClusterByRole(ctx, cid, hosts.RoleClickHouse, &pillar)
	if err != nil {
		return subCluster{}, err
	}

	return subCluster{
		SubCluster: sc,
		Pillar:     &pillar,
	}, nil
}

func (ch *ClickHouse) chSubClusterAtRevision(ctx context.Context, reader clusterslogic.Reader, cid string, rev int64) (subCluster, error) {
	var pillar chpillars.SubClusterCH
	sc, err := reader.SubClusterByRoleAtRevision(ctx, cid, hosts.RoleClickHouse, &pillar, rev)
	if err != nil {
		return subCluster{}, err
	}

	return subCluster{
		SubCluster: sc,
		Pillar:     &pillar,
	}, nil
}

func (ch *ClickHouse) zkSubCluster(ctx context.Context, reader clusterslogic.Reader, cid string) (zkSubCluster, error) {
	var pillar chpillars.SubClusterZK
	sc, err := reader.SubClusterByRole(ctx, cid, hosts.RoleZooKeeper, &pillar)
	if err != nil {
		return zkSubCluster{}, err
	}

	return zkSubCluster{
		SubCluster: sc,
		Pillar:     &pillar,
	}, nil
}

func (ch *ClickHouse) chShard(ctx context.Context, reader clusterslogic.Reader, shardID string) (shard, error) {
	var pillar chpillars.ShardCH
	s, err := reader.ShardByShardID(ctx, shardID, &pillar)
	if err != nil {
		return shard{}, err
	}

	return shard{
		Shard:  s,
		Pillar: &pillar,
	}, nil
}

// createHosts creates hosts for single shard or subcluster
func (ch *ClickHouse) createHosts(
	ctx context.Context,
	session sessions.Session,
	modifier creatorModifier,
	cluster cluster,
	subCID string,
	shardID optional.String,
	shardName string,
	specs []chmodels.HostSpec,
	subnets []networkProvider.Subnet,
	resolvedGroup clusterslogic.ResolvedHostGroup,
) ([]hosts.Host, error) {
	hostArgs := make([]models.AddHostArgs, 0, len(specs))

	fqdnMap, err := modifier.GenerateSemanticFQDNs(
		cluster.Pillar.Data.CloudType,
		cluster.Type,
		resolvedGroup.HostsToAdd,
		resolvedGroup.HostsCurrent,
		shardName, cluster.ClusterID, resolvedGroup.TargetResourcePreset().VType, compute.Ubuntu,
	)
	if err != nil {
		return nil, err
	}

	for _, spec := range specs {
		fqdn := fqdnMap[spec.ZoneID][0]
		fqdnMap[spec.ZoneID] = fqdnMap[spec.ZoneID][1:]

		optSubnet := optional.NewString(spec.SubnetID)
		if len(spec.SubnetID) == 0 {
			optSubnet.Valid = false
		}

		subnet, err := ch.compute.PickSubnet(ctx, subnets, resolvedGroup.TargetResourcePreset().VType, spec.ZoneID, spec.AssignPublicIP, optSubnet, session.FolderCoords.FolderExtID)
		if err != nil {
			return nil, err
		}

		hostArgs = append(hostArgs, models.AddHostArgs{
			ClusterID:        cluster.ClusterID,
			SubClusterID:     subCID,
			ShardID:          shardID,
			ResourcePresetID: resolvedGroup.TargetResourcePreset().ID,
			ZoneID:           spec.ZoneID,
			FQDN:             fqdn,
			DiskTypeExtID:    resolvedGroup.DiskTypeExtID,
			SpaceLimit:       resolvedGroup.TargetDiskSize(),
			SubnetID:         subnet.ID,
			AssignPublicIP:   spec.AssignPublicIP,
			Revision:         cluster.Revision,
		})
	}

	// For tests stability
	sort.Slice(hostArgs, func(i, j int) bool {
		return hostArgs[i].FQDN < hostArgs[j].FQDN
	})

	return modifier.AddHosts(ctx, hostArgs)
}

func getZkHostTaskArg(zkHosts []hosts.HostExtended) string {
	b := strings.Builder{}
	for i, host := range zkHosts {
		b.WriteString(host.FQDN)
		b.WriteString(":2181")
		if i != len(zkHosts) {
			b.WriteRune(',')
		}
	}

	return b.String()
}

func (ch *ClickHouse) getDefaultClickHouseResources(modifier prepareModifier) (models.ClusterResources, error) {
	preset, err := modifier.ResourcePresetFromDefaultConfig(
		clusters.TypeClickHouse,
		hosts.RoleClickHouse,
	)
	if err != nil {
		return models.ClusterResources{}, err
	}

	result := models.ClusterResources{
		ResourcePresetExtID: preset.ExtID,
		DiskTypeExtID:       preset.DiskTypeExtID,
		DiskSize:            preset.DiskSizes[0],
	}

	return result, nil
}

func buildClickHouseHostGroups(chHosts []hosts.HostExtended, modifiedShard string, hostsToAdd, hostsToRemove clusterslogic.ZoneHostsList) []clusterslogic.HostGroup {
	shardHostsMap := hosts.SplitHostsExtendedByShard(chHosts)

	var result = make([]clusterslogic.HostGroup, 0, len(shardHostsMap))
	for shard, shardHosts := range shardHostsMap {
		group := clusterslogic.HostGroup{
			Role:                       hosts.RoleClickHouse,
			CurrentResourcePresetExtID: optional.NewString(shardHosts[0].ResourcePresetExtID),
			DiskTypeExtID:              shardHosts[0].DiskTypeExtID,
			CurrentDiskSize:            optional.NewInt64(shardHosts[0].SpaceLimit),
			HostsCurrent:               clusterslogic.ZoneHostsListFromHosts(shardHosts),
			ShardName:                  optional.NewString(shard),
		}

		if modifiedShard == shard {
			group.HostsToAdd = hostsToAdd
			group.HostsToRemove = hostsToRemove
		}

		result = append(result, group)
	}

	return result
}

func buildZooKeeperHostGroup(zkHosts []hosts.HostExtended, hostsToAdd, hostsToRemove clusterslogic.ZoneHostsList) clusterslogic.HostGroup {
	result := clusterslogic.HostGroup{
		Role:          hosts.RoleZooKeeper,
		HostsToAdd:    hostsToAdd,
		HostsToRemove: hostsToRemove,
	}

	if len(zkHosts) > 0 {
		result.CurrentResourcePresetExtID = optional.NewString(zkHosts[0].ResourcePresetExtID)
		result.DiskTypeExtID = zkHosts[0].DiskTypeExtID
		result.CurrentDiskSize = optional.NewInt64(zkHosts[0].SpaceLimit)
		result.HostsCurrent = clusterslogic.ZoneHostsListFromHosts(zkHosts)
	}

	return result
}

func buildClusterHostGroups(chHosts, zkHosts []hosts.HostExtended, chModifiedShard optional.String, chHostsToAdd, chHostsToRemove,
	zkHostsToAdd, zkHostsToRemove clusterslogic.ZoneHostsList) []clusterslogic.HostGroup {
	chGroups := buildClickHouseHostGroups(chHosts, chModifiedShard.String, chHostsToAdd, chHostsToRemove)
	if len(zkHosts) == 0 {
		return chGroups
	}

	return append(chGroups, buildZooKeeperHostGroup(zkHosts, zkHostsToAdd, zkHostsToRemove))
}

func (ch *ClickHouse) getMinimalZKCores(chHostGroups []clusterslogic.ResolvedHostGroup) float64 {
	var chCores float64
	for _, group := range chHostGroups {
		chCores += group.TargetResourcePreset().CPULimit * float64(len(group.HostsCurrent)+len(group.HostsToAdd)-len(group.HostsToRemove))
	}

	var minCores float64
	for _, res := range ch.cfg.ClickHouse.MinimalZkResources {
		if chCores >= res.ChSubclusterCPU {
			minCores = res.ZKHostCPU
		}
	}

	return minCores
}

func (ch *ClickHouse) getDefaultZookeeperResources(ctx context.Context, session sessions.Session, modifier creatorModifier,
	chHostGroups []clusterslogic.ResolvedHostGroup, zones []string, flavorType string) (models.ClusterResources, error) {
	preset, err := modifier.ResourcePresetByCPU(ctx, clusters.TypeClickHouse, hosts.RoleZooKeeper, optional.NewString(flavorType), optional.Int64{},
		ch.getMinimalZKCores(chHostGroups), zones, session.FeatureFlags.List())
	if err != nil {
		return models.ClusterResources{}, err
	}
	result := models.ClusterResources{
		ResourcePresetExtID: preset.ExtID,
		DiskTypeExtID:       preset.DiskTypeExtID,
	}
	if preset.DiskSizeRange.Valid {
		result.DiskSize = preset.DiskSizeRange.IntervalInt64.Min()
	} else {
		result.DiskSize = preset.DiskSizes[0]
	}
	return result, nil
}

func (ch *ClickHouse) buildZookeeperResources(
	ctx context.Context,
	session sessions.Session,
	modifier creatorModifier,
	chHostGroups []clusterslogic.ResolvedHostGroup,
	hostSpecs chmodels.ClusterHosts,
	currentHosts []hosts.HostExtended,
	resSpec models.ClusterResourcesSpec,
	region optional.String,
	subnets []networkProvider.Subnet) (models.ClusterResources, []chmodels.HostSpec, error) {
	zkZones := chmodels.ZonesFromHostSpecs(hostSpecs.ZooKeeperNodes)
	if len(zkZones) == 0 {
		zkZones = chmodels.ZonesFromHosts(currentHosts, hostSpecs.ClickHouseNodes)
	}

	zkResources, err := ch.getDefaultZookeeperResources(ctx, session, modifier, chHostGroups, zkZones, chmodels.StandardFlavorType)
	if err != nil {
		return models.ClusterResources{}, nil, err
	}

	zkResources.Merge(resSpec)

	if len(hostSpecs.ZooKeeperNodes) == 0 {
		zkZones, err = selectHostZones(ctx, session, modifier, ch.cfg.ClickHouse.ZooKeeperHostCount, zkZones, region)
		if err != nil {
			return models.ClusterResources{}, nil, err
		}

		for _, zone := range zkZones {
			hostSpecs.ZooKeeperNodes = append(hostSpecs.ZooKeeperNodes, chmodels.HostSpec{
				ZoneID:         zone,
				AssignPublicIP: false,
				HostRole:       hosts.RoleZooKeeper,
				SubnetID:       chmodels.SelectSubnetForZone(zone, subnets, hostSpecs.ClickHouseNodes),
			})
		}
	}

	return zkResources, hostSpecs.ZooKeeperNodes, nil
}

func (ch *ClickHouse) validateZookeeperCores(ctx context.Context, session sessions.Session, modifier creatorModifier, resolvedHostGroups clusterslogic.ResolvedHostGroups, op string) error {
	zk, ok := resolvedHostGroups.ByHostRole(hosts.RoleZooKeeper)
	if !ok {
		return nil
	}

	minCores := ch.getMinimalZKCores(resolvedHostGroups.MustGroupsByHostRole(hosts.RoleClickHouse))
	zkPreset := zk.TargetResourcePreset()
	if zkPreset.CPULimit < minCores {
		preset, err := modifier.ResourcePresetByCPU(ctx, clusters.TypeClickHouse, hosts.RoleZooKeeper, optional.NewString(zkPreset.Type),
			optional.NewInt64(zkPreset.Generation), minCores, []string{}, session.FeatureFlags.List())
		if err != nil {
			return err
		}

		msg := fmt.Sprintf("The resource preset of ZooKeeper hosts must have at least %d CPU cores (%s or higher) for the requested cluster configuration.", int(minCores), preset.ExtID)
		if op != "" {
			msg = fmt.Sprintf("ZooKeeper hosts must be upscaled in order to %s. %s", op, msg)
		}

		return semerr.FailedPrecondition(msg)
	}

	return nil
}

func (ch *ClickHouse) validateClickHouseVersion(session sessions.Session, version string) (string, error) {
	for _, ver := range ch.cfg.ClickHouse.Versions {
		if (version == "" && ver.Default) || (version != "" && strings.HasPrefix(ver.ID, version)) {
			if ver.Deprecated {
				return "", semerr.InvalidInputf("can't create cluster, version %q is deprecated", version)
			}

			return ver.ID, nil
		}
	}

	if session.FeatureFlags.Has(chmodels.ClickHouseTestingVersionsFeatureFlag) {
		// TODO validate version in Packages.gz
		return version, nil
	}

	return "", semerr.NotFoundf("version %q not found", version)
}

func (ch *ClickHouse) getSourceOrMinimalVersion(source string) (string, error) {
	minVer := ch.cfg.ClickHouse.Versions[0]
	minMaj, minMin, err := chmodels.ParseVersion(ch.cfg.ClickHouse.Versions[0].ID)
	if err != nil {
		return "", nil
	}

	for _, v := range ch.cfg.ClickHouse.Versions {
		res, err := chmodels.VersionGreaterOrEqual(v.ID, minMaj, minMin)
		if err != nil {
			return "", err
		}

		if !res {
			minVer = v
			minMaj, minMin, err = chmodels.ParseVersion(v.ID)
			if err != nil {
				return "", nil
			}
		}
	}

	ok, err := chmodels.VersionGreaterOrEqual(source, minMaj, minMin)
	if err != nil {
		return "", err
	}

	if ok {
		return chmodels.CutVersionToMajor(source), nil
	}

	return chmodels.CutVersionToMajor(minVer.ID), nil
}

func getTimeout(timeout optional.Duration, args map[string]interface{}, hostGroups clusterslogic.ResolvedHostGroups) time.Duration {
	var extend int64 = 1
	for _, group := range hostGroups.MustGroupsByHostRole(hosts.RoleClickHouse) {
		if group.TargetResourcePreset().CPUGuarantee < 1 {
			extend = 2
			break
		}
	}

	defaultTimeLimit := time.Hour
	var timeLimit time.Duration
	if timeout.Valid {
		timeLimit = timeout.Must()
	} else {
		timeLimit = defaultTimeLimit
	}

	if v, ok := args["restart"]; ok && v.(bool) {
		var hostCount int64 = 0
		for _, group := range hostGroups.MustGroupsByHostRole(hosts.RoleClickHouse) {
			hostCount += group.HostsToAdd.Len() + group.HostsCurrent.Len() - group.HostsToRemove.Len()
		}

		if groups, ok := hostGroups.GroupsByHostRole(hosts.RoleZooKeeper); ok {
			if len(groups) != 1 {
				panic(fmt.Sprintf("multiple zk host groups %+v", hostGroups))
			}
			hostCount += groups[0].HostsToAdd.Len() + groups[0].HostsCurrent.Len() - groups[0].HostsToRemove.Len()
		}

		if time.Duration(hostCount*5)*time.Minute-time.Hour > 0 {
			timeLimit += time.Duration(hostCount*5)*time.Minute - time.Hour
		}
	}

	return timeLimit * time.Duration(extend)
}

func getAvailableZones(ctx context.Context, session sessions.Session, modifier creatorModifier, region optional.String) ([]string, error) {
	availableZones, err := modifier.ListAvailableZones(ctx, session, true)
	if err != nil {
		return nil, err
	}

	availableZonesNames := make([]string, 0, len(availableZones))
	for _, zone := range availableZones {
		if !region.Valid || zone.RegionID == region.String {
			availableZonesNames = append(availableZonesNames, zone.Name)
		}
	}

	return availableZonesNames, nil
}

func selectHostZones(ctx context.Context, session sessions.Session, modifier creatorModifier, hostCount int, preferredZones []string, region optional.String) ([]string, error) {
	resultZones := preferredZones
	if len(resultZones) >= hostCount {
		return resultZones[:hostCount], nil
	}

	availableZones, err := getAvailableZones(ctx, session, modifier, region)
	if err != nil {
		return nil, err
	}

	if region.Valid && len(availableZones) == 0 {
		return nil, semerr.NotFoundf("region %q is not available", region.String)
	}

	preferredZonesMap := map[string]struct{}{}
	for _, preferredZone := range preferredZones {
		preferredZonesMap[preferredZone] = struct{}{}
	}

	var unusedZones []string
	for _, zone := range availableZones {
		if _, ok := preferredZonesMap[zone]; !ok {
			unusedZones = append(unusedZones, zone)
		}
	}

	rnd := rand.NewSource(time.Now().Unix())
	slices.Shuffle(unusedZones, rnd)
	if len(unusedZones) > hostCount-len(resultZones) {
		unusedZones = unusedZones[:hostCount-len(resultZones)]
	}

	resultZones = append(resultZones, unusedZones...)

	if len(preferredZones) == 0 {
		preferredZones = availableZones
	}

	for len(resultZones) < hostCount {
		resultZones = append(resultZones, preferredZones[int(rnd.Int63())%len(preferredZones)])
	}

	return resultZones, nil
}

//Selects new Keepers from all hosts, trying to make zone-balanced setup
func chooseNewKeeperHosts(allHosts []hosts.Host, existingKeepers []hosts.Host) []hosts.Host {
	zkHostCount := 1
	if len(allHosts) >= 3 {
		zkHostCount = 3
	}
	zkHostCount -= len(existingKeepers)
	groupByZone := map[string][]hosts.Host{}
	zones := make([]string, 0)

	existingKeeperFQDNs := hosts.GetFQDNs(existingKeepers)

	for _, h := range allHosts {
		if slices.ContainsString(existingKeeperFQDNs, h.FQDN) {
			continue
		}
		_, isset := groupByZone[h.ZoneID]
		if !isset {
			zones = append(zones, h.ZoneID)
		}
		groupByZone[h.ZoneID] = append(groupByZone[h.ZoneID], h)
	}

	//sorting zones list to put these, where keepers already present, to the end of the list
	existingKeeperZones := hosts.GetUniqueZones(existingKeepers)
	sort.Slice(zones, func(i, j int) bool {
		return slices.ContainsString(existingKeeperZones, zones[i]) || slices.ContainsString(existingKeeperZones, zones[j])
	})

	var keeperHosts []hosts.Host
	for len(keeperHosts) < zkHostCount {
		for _, zone := range zones {
			if len(groupByZone[zone]) > 0 {
				var host hosts.Host
				host, groupByZone[zone] = groupByZone[zone][0], groupByZone[zone][1:]
				keeperHosts = append(keeperHosts, host)
				if len(keeperHosts) >= zkHostCount {
					break
				}
			}
		}
	}
	return keeperHosts
}

func deletedHostsToArgs(chHosts []hosts.Host, regionID string) []map[string]string {
	result := make([]map[string]string, 0, len(chHosts))
	for _, host := range chHosts {
		result = append(result, map[string]string{
			"fqdn":        host.FQDN,
			"vtype":       host.VType,
			"vtype_id":    host.VTypeID.String,
			"region_name": regionID,
		})
	}

	return result
}

func (ch *ClickHouse) validateGeoBaseURI(ctx context.Context, uri optional.String) error {
	if !uri.Valid {
		return nil
	}

	return validators.MustExternalResourceURIValidator(ctx, ch.uriValidator, "geobase",
		ch.cfg.ClickHouse.ExternalURIValidation.Regexp, ch.cfg.ClickHouse.ExternalURIValidation.Message).ValidateString(uri.String)
}
