package provider

import (
	"context"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/clickhouse"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/clickhouse/chmodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/common"
	clusterslogic "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/sessions"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
	modelsoptional "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/optional"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/slices"
)

const (
	updateClusterDefaultTimeout = time.Hour
)

type clickHouseChanges struct {
	common.ClusterChanges

	// Need to update subcluster pillar
	HasPillarChanges bool
	// Whether hosts added/removed/changed
	HasResourceChanges bool
}

func newChanges() clickHouseChanges {
	return clickHouseChanges{ClusterChanges: common.GetClusterChanges()}
}

type clickHouseResourceChanges struct {
	shardsToDelete []clusters.Shard
	hostsToDelete  []string
	hostsToAdd     map[string][]chmodels.HostSpec
	shardsToAdd    []ShardSpec
}

func (c *clickHouseChanges) mergeChanges(other clickHouseChanges) {
	c.NeedUpgrade = c.NeedUpgrade || other.NeedUpgrade
	c.HasChanges = c.HasChanges || other.HasChanges
	c.HasMetadataChanges = c.HasMetadataChanges || other.HasMetadataChanges
	c.HasMetaDBChanges = c.HasMetaDBChanges || other.HasMetaDBChanges
	c.HasPillarChanges = c.HasPillarChanges || other.HasPillarChanges
	c.HasResourceChanges = c.HasResourceChanges || other.HasResourceChanges

	c.Timeout += other.Timeout
	for param, value := range other.TaskArgs {
		c.TaskArgs[param] = value
	}
}

func (ch *ClickHouse) ModifyDataCloudCluster(ctx context.Context, args clickhouse.UpdateDataCloudClusterArgs) (operations.Operation, error) {
	if args.Name.Valid {
		if err := models.ClusterNameValidator.ValidateString(args.Name.String); err != nil {
			return operations.Operation{}, err
		}
	}

	return ch.operator.ModifyOnCluster(ctx, args.ClusterID, clusters.TypeClickHouse,
		func(ctx context.Context,
			session sessions.Session,
			reader clusterslogic.Reader,
			modifier clusterslogic.Modifier,
			cluster clusterslogic.Cluster,
		) (operations.Operation, error) {
			clusterChanges := newChanges()
			clusterChanges.Timeout = updateClusterDefaultTimeout
			var err error

			clusterChanges.HasMetadataChanges, err = modifier.ModifyClusterMetadata(ctx, cluster, args.Name, modelsoptional.Labels{})
			if err != nil {
				return operations.Operation{}, err
			}

			if args.Description.Valid {
				changes, err := modifier.ModifyClusterMetadataParameters(ctx, cluster, args.Description, modelsoptional.Labels{}, optional.Bool{}, modelsoptional.MaintenanceWindow{})
				if err != nil {
					return operations.Operation{}, err
				}

				clusterChanges.HasMetadataChanges = clusterChanges.HasMetadataChanges || changes
			}

			chCluster, err := ch.chCluster(ctx, reader, cluster.ClusterID)
			if err != nil {
				return operations.Operation{}, err
			}

			chSubCluster, err := ch.chSubCluster(ctx, reader, cluster.ClusterID)
			if err != nil {
				return operations.Operation{}, err
			}

			versionChanges, err := ch.upgradeClickHouseVersion(session, args.ConfigSpec.Version, &chSubCluster)
			if err != nil {
				return operations.Operation{}, err
			}
			clusterChanges.mergeChanges(versionChanges)

			if args.ConfigSpec.Access.DataTransfer.Valid || args.ConfigSpec.Access.DataLens.Valid ||
				args.ConfigSpec.Access.Ipv4CidrBlocks != nil || args.ConfigSpec.Access.Ipv6CidrBlocks != nil {
				if err := args.ConfigSpec.Access.ValidateAndSane(); err != nil {
					return operations.Operation{}, err
				}
				chSubCluster.Pillar.SetAccess(args.ConfigSpec.Access)
				clusterChanges.mergeChanges(clickHouseChanges{
					ClusterChanges: common.ClusterChanges{
						TaskArgs:   map[string]interface{}{"update-firewall": true},
						HasChanges: true,
					},
					HasPillarChanges: true,
				})
			}

			changes, newClusterHosts, err := ch.updateDataCloudResources(ctx, session, reader, modifier, chCluster, args.ConfigSpec.Resources, &chSubCluster)
			if err != nil {
				return operations.Operation{}, err
			}
			clusterChanges.mergeChanges(changes)

			if newClusterHosts != nil {
				keeperHosts := chSubCluster.Pillar.Data.ClickHouse.ZKHosts
				if len(chSubCluster.Pillar.Data.ClickHouse.KeeperHosts) > 0 {
					keeperHosts = []string{}
					for keeperHost := range chSubCluster.Pillar.Data.ClickHouse.KeeperHosts {
						keeperHosts = append(keeperHosts, keeperHost)
					}
				}

				result, oldKeepers, newKeepers := updateKeeperHosts(keeperHosts, newClusterHosts)

				var allKeepers []hosts.Host
				allKeepers = append(allKeepers, oldKeepers...)
				allKeepers = append(allKeepers, newKeepers...)

				if len(chSubCluster.Pillar.Data.ClickHouse.KeeperHosts) > 0 {
					chSubCluster.Pillar.SetEmbeddedKeeperHosts(allKeepers)
				} else {
					chSubCluster.Pillar.SetEmbeddedZKHosts(allKeepers)
				}

				clusterChanges.mergeChanges(clickHouseChanges{
					ClusterChanges: common.ClusterChanges{
						TaskArgs: map[string]interface{}{
							"keeper_hosts": getKeeperModifyTaskArgs(oldKeepers, newKeepers),
							"restart":      result,
						},
					},
					HasPillarChanges: result,
				})
			}

			return ch.applyChanges(ctx, session, modifier, clusterChanges, chCluster.Cluster, chSubCluster, optional.Strings{})
		})
}

func getKeeperModifyTaskArgs(oldKeepers []hosts.Host, newKeepers []hosts.Host) map[string]bool {
	keeperHostArgs := make(map[string]bool, len(oldKeepers)+len(newKeepers))
	for _, fqdn := range hosts.GetFQDNs(oldKeepers) {
		keeperHostArgs[fqdn] = true
	}

	for _, fqdn := range hosts.GetFQDNs(newKeepers) {
		keeperHostArgs[fqdn] = false
	}
	return keeperHostArgs
}

func updateKeeperHosts(keeperFQDNs []string, newClusterHosts []hosts.Host) (bool, []hosts.Host, []hosts.Host) {
	var existingKeeperHosts []hosts.Host
	for _, host := range newClusterHosts {
		if slices.ContainsString(keeperFQDNs, host.FQDN) {
			existingKeeperHosts = append(existingKeeperHosts, host)
		}
	}

	//Now there are 2 or 1 hosts and 3-keeper configuration. Downscaling to 1-keeper configuration
	if len(newClusterHosts) < 3 && len(keeperFQDNs) == 3 {
		// choosing the first one of present keeper nodes
		return true, []hosts.Host{existingKeeperHosts[0]}, []hosts.Host{}
	}

	/*
		  Covers 2 cases:
			- Now there are 3+ hosts and 1-keeper configuration. Upscaling to 3-keeper configuration
			- Now there are 3+ hosts and 3-keeper configuration, but some of keeper nodes were deleted.
			  Choosing new ones
	*/
	if len(newClusterHosts) >= 3 && len(existingKeeperHosts) < 3 {
		existingKeeperZones := hosts.GetUniqueZones(existingKeeperHosts)
		totalZones := hosts.GetUniqueZones(newClusterHosts)
		var newKeeperHosts []hosts.Host
		var existingKeepers []hosts.Host
		if len(existingKeeperZones) == 1 && len(totalZones) > 2 {
			//using only existing keeper to make setup more zone-balanced
			existingKeepers = []hosts.Host{existingKeeperHosts[0]}
		} else {
			//reusing all existing keepers
			existingKeepers = existingKeeperHosts
		}
		newKeeperHosts = chooseNewKeeperHosts(newClusterHosts, existingKeepers)
		return true, existingKeepers, newKeeperHosts
	}

	//Trying to make Keepers zone-balanced if they are not
	if len(newClusterHosts) >= 3 && len(existingKeeperHosts) == 3 {
		existingKeeperZones := hosts.GetUniqueZones(existingKeeperHosts)
		newHostsZones := hosts.GetUniqueZones(newClusterHosts)
		if len(existingKeeperZones) < 3 && len(newHostsZones) > len(existingKeeperZones) {
			// reusing only one keeper from each zone
			zoneToKeeperHosts := make(map[string][]hosts.Host)
			for _, keeperHost := range existingKeeperHosts {
				zoneToKeeperHosts[keeperHost.ZoneID] = append(zoneToKeeperHosts[keeperHost.ZoneID], keeperHost)
			}

			var keeperHostsToReuse []hosts.Host
			for zone := range zoneToKeeperHosts {
				keeperHostsToReuse = append(keeperHostsToReuse, zoneToKeeperHosts[zone][0])
			}
			return true, keeperHostsToReuse, chooseNewKeeperHosts(newClusterHosts, keeperHostsToReuse)
		}
	}

	return false, existingKeeperHosts, []hosts.Host{}
}

func (ch *ClickHouse) updateDataCloudResources(
	ctx context.Context,
	session sessions.Session,
	reader clusterslogic.Reader,
	modifier clusterslogic.Modifier,
	cluster cluster,
	resources chmodels.DataCloudResources,
	subCluster *subCluster,
) (clickHouseChanges, []hosts.Host, error) {
	presentHosts, err := clusterslogic.ListAllHosts(ctx, reader, cluster.ClusterID)
	if err != nil {
		return clickHouseChanges{}, nil, err
	}

	chHosts := clusterslogic.GetHostsWithRole(presentHosts, hosts.RoleClickHouse)
	shardHostMap := chmodels.SplitHostsExtendedByShard(chHosts)
	shards, err := reader.ListShards(ctx, cluster.ClusterID)
	if err != nil {
		return clickHouseChanges{}, nil, err
	}

	shardNameMap := map[string]string{}
	for _, shard := range shards {
		shardNameMap[shard.ShardID] = shard.Name
	}

	hostGroupMap := map[string]*clusterslogic.HostGroup{}
	for shard, shardHosts := range shardHostMap {
		host := shardHosts[0]
		hostGroupMap[shard] = &clusterslogic.HostGroup{
			Role:                       hosts.RoleClickHouse,
			CurrentResourcePresetExtID: optional.NewString(host.ResourcePresetExtID),
			NewResourcePresetExtID:     resources.ResourcePresetID,
			DiskTypeExtID:              host.DiskTypeExtID,
			CurrentDiskSize:            optional.NewInt64(host.SpaceLimit),
			NewDiskSize:                resources.DiskSize,
			HostsCurrent:               clusterslogic.ZoneHostsListFromHosts(shardHosts),
			ShardName:                  optional.NewString(shardNameMap[shard]),
		}
	}

	changes := newChanges()

	resourceChanges, err := ch.planResourceChanges(ctx, session, modifier, cluster, resources, hostGroupMap, shardHostMap, shards)
	if err != nil {
		return clickHouseChanges{}, nil, err
	}

	keeperHosts := subCluster.Pillar.Data.ClickHouse.ZKHosts
	if len(subCluster.Pillar.Data.ClickHouse.KeeperHosts) > 0 {
		keeperHosts = []string{}
		for keeperHost := range subCluster.Pillar.Data.ClickHouse.KeeperHosts {
			keeperHosts = append(keeperHosts, keeperHost)
		}
	}

	err = validateKeeperRemoval(resourceChanges, shardHostMap, keeperHosts)
	if err != nil {
		return clickHouseChanges{}, nil, err
	}

	hostGroups := make([]clusterslogic.HostGroup, 0, len(hostGroupMap))
	for _, group := range hostGroupMap {
		hostGroups = append(hostGroups, *group)
	}

	resolvedHostGroups, hasChanges, err := modifier.ValidateResources(ctx, session, clusters.TypeClickHouse, hostGroups...)
	changes.HasChanges = hasChanges
	changes.HasResourceChanges = hasChanges
	if err != nil || !hasChanges {
		return changes, nil, err
	}

	resultHosts, newChanges, err := ch.applyResourceChanges(ctx, session, modifier, cluster, *subCluster, shardHostMap, shardNameMap, resolvedHostGroups, resourceChanges, chHosts)
	if err != nil {
		return clickHouseChanges{}, nil, err
	}
	changes.mergeChanges(newChanges)

	changes.Timeout += resolvedHostGroups.ExtraTimeout()

	return changes, resultHosts, nil
}

func validateKeeperRemoval(
	resourceChanges clickHouseResourceChanges,
	shardHostMap map[string][]hosts.HostExtended,
	keeperFQDNs []string) error {

	var totalFQDNsToDelete []string
	for _, shard := range resourceChanges.shardsToDelete {
		for _, host := range shardHostMap[shard.ShardID] {
			totalFQDNsToDelete = append(totalFQDNsToDelete, host.FQDN)
		}
	}
	totalFQDNsToDelete = append(totalFQDNsToDelete, resourceChanges.hostsToDelete...)
	keepersToRemove := 0
	for _, keeperFQDN := range keeperFQDNs {
		if slices.ContainsString(totalFQDNsToDelete, keeperFQDN) {
			keepersToRemove++
		}
	}
	if keepersToRemove == len(keeperFQDNs) {
		return xerrors.Errorf("All Keeper hosts will be deleted")
	}
	return nil
}

func (ch *ClickHouse) planResourceChanges(
	ctx context.Context,
	session sessions.Session,
	modifier clusterslogic.Modifier,
	cluster cluster,
	resources chmodels.DataCloudResources,
	hostGroupMap map[string]*clusterslogic.HostGroup,
	shardHostMap map[string][]hosts.HostExtended,
	shards []clusters.Shard,
) (clickHouseResourceChanges, error) {
	availableZones, err := getAvailableZones(ctx, session, modifier, optional.NewString(cluster.Pillar.Data.RegionID))
	if err != nil {
		return clickHouseResourceChanges{}, err
	}

	// Delete ClickHouse shards
	chShardsToDelete, err := ch.planDeleteClickHouseShards(resources, hostGroupMap, shards)
	if err != nil {
		return clickHouseResourceChanges{}, err
	}

	// Delete ClickHouse replicas
	chHostsToDelete, err := planDeleteClickHouseReplicas(hostGroupMap, shardHostMap, resources.ReplicaCount)
	if err != nil {
		return clickHouseResourceChanges{}, err
	}

	// Create ClickHouse replicas
	chHostToAdd := planAddClickHouseReplicas(availableZones, hostGroupMap, resources.ReplicaCount)

	// Create ClickHouse shards
	chShardToAdd, err := ch.planAddClickHouseShards(session, resources, hostGroupMap, shards)
	if err != nil {
		return clickHouseResourceChanges{}, err
	}

	return clickHouseResourceChanges{
		shardsToDelete: chShardsToDelete,
		hostsToDelete:  chHostsToDelete,
		hostsToAdd:     chHostToAdd,
		shardsToAdd:    chShardToAdd,
	}, nil
}

func (ch *ClickHouse) applyResourceChanges(
	ctx context.Context,
	session sessions.Session,
	modifier clusterslogic.Modifier,
	cluster cluster,
	subCluster subCluster,
	shardHostMap map[string][]hosts.HostExtended,
	shardNameMap map[string]string,
	resolvedHostGroups clusterslogic.ResolvedHostGroups,
	changes clickHouseResourceChanges,
	chHosts []hosts.HostExtended,
) ([]hosts.Host, clickHouseChanges, error) {
	hostGroupMap := resolvedHostGroups.MustMapByShardName(hosts.RoleClickHouse)

	hostsToDelete := make([]hosts.Host, 0, len(changes.hostsToDelete))
	hostsToAdd := make([]hosts.Host, 0, len(changes.hostsToAdd))
	isDownscale := false

	// Delete ClickHouse shards
	for _, shard := range changes.shardsToDelete {
		if err := deleteShard(ctx, modifier, cluster, subCluster, shard); err != nil {
			return []hosts.Host{}, clickHouseChanges{}, err
		}

		hostsToDelete = append(hostsToDelete, hosts.HostsExtendedToHosts(shardHostMap[shard.ShardID])...)
	}

	// Delete ClickHouse replicas
	if len(changes.hostsToDelete) > 0 {
		deletedReplicas, err := modifier.DeleteHosts(ctx, cluster.ClusterID, changes.hostsToDelete, cluster.Revision)
		if err != nil {
			return []hosts.Host{}, clickHouseChanges{}, err
		}

		hostsToDelete = append(hostsToDelete, deletedReplicas...)
	}

	// Update Clickhouse hosts
	var fqdnsToDelete []string
	for _, host := range hostsToDelete {
		fqdnsToDelete = append(fqdnsToDelete, host.FQDN)
	}

	for _, host := range chHosts {
		if slices.ContainsString(fqdnsToDelete, host.FQDN) {
			continue
		}

		group, ok := hostGroupMap[shardNameMap[host.ShardID.String]]
		if !ok {
			return []hosts.Host{}, clickHouseChanges{}, xerrors.Errorf("host group for host %q not found", host.FQDN)
		}
		isDownscale = isDownscale || group.IsDownscale()
		args := models.ModifyHostArgs{
			FQDN:             host.FQDN,
			ClusterID:        cluster.ClusterID,
			Revision:         cluster.Revision,
			SpaceLimit:       group.TargetDiskSize(),
			ResourcePresetID: group.TargetResourcePreset().ID,
			DiskTypeExtID:    group.DiskTypeExtID,
		}

		if err := modifier.ModifyHost(ctx, args); err != nil {
			return []hosts.Host{}, clickHouseChanges{}, err
		}
	}

	_, subnets, err := ch.compute.NetworkAndSubnets(ctx, cluster.NetworkID)
	if err != nil {
		return nil, clickHouseChanges{}, err
	}

	// Create ClickHouse replicas
	for shard := range shardHostMap {
		shardName := shardNameMap[shard]
		createdHosts, err := ch.createHosts(
			ctx,
			session,
			modifier,
			cluster,
			subCluster.SubClusterID,
			optional.NewString(shard),
			shardName,
			changes.hostsToAdd[shard],
			subnets,
			hostGroupMap[shardName],
		)
		if err != nil {
			return []hosts.Host{}, clickHouseChanges{}, err
		}
		hostsToAdd = append(hostsToAdd, createdHosts...)
	}

	//Create ClickHouse shards
	newHosts, err := ch.addShards(ctx, session, modifier, hostGroupMap, cluster, subCluster, changes.shardsToAdd)
	if err != nil {
		return []hosts.Host{}, clickHouseChanges{}, err
	}
	hostsToAdd = append(hostsToAdd, newHosts...)

	var resultHosts []hosts.Host
	for _, host := range chHosts {
		if slices.ContainsString(fqdnsToDelete, host.FQDN) {
			continue
		}
		resultHosts = append(resultHosts, host.Host)
	}
	resultHosts = append(resultHosts, hostsToAdd...)

	hostsToAddFQDNs := hosts.GetFQDNs(hostsToAdd)
	return resultHosts, clickHouseChanges{
		ClusterChanges: common.ClusterChanges{
			HasChanges: len(hostsToAdd)+len(hostsToDelete) > 0,
			TaskArgs: map[string]interface{}{
				"hosts_to_create":      hostsToAddFQDNs,
				"hosts_to_delete":      deletedHostsToArgs(hostsToDelete, cluster.Pillar.Data.RegionID),
				"resetup_from_replica": len(hostsToAddFQDNs) > 0,
				"reverse_order":        isDownscale,
			},
		},
		HasPillarChanges: len(changes.shardsToAdd)+len(changes.shardsToDelete) > 0,
	}, err
}
