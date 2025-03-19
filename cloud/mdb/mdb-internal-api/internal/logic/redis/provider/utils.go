package provider

import (
	"context"
	"fmt"
	"sort"

	"a.yandex-team.ru/cloud/mdb/internal/compute"
	networkProvider "a.yandex-team.ru/cloud/mdb/internal/network"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	clusterslogic "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/sessions"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/redis/provider/internal/rpillars"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/redis/rmodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/environment"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts/services"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/pillars"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func GetMasters(currentHosts []hosts.HostExtended) []string {
	var masters []string
	for _, host := range currentHosts {
		for _, srv := range host.Health.Services {
			if srv.Role == services.RoleMaster {
				masters = append(masters, host.FQDN)
			}
		}
	}
	return masters
}

func buildHostGroups(rHosts []hosts.HostExtended, modifiedShard string, hostsToAdd, hostsToRemove clusterslogic.ZoneHostsList) (int, []clusterslogic.HostGroup) {
	shardHostsMap := hosts.SplitHostsExtendedByShard(rHosts)

	var result = make([]clusterslogic.HostGroup, 0, len(shardHostsMap))
	for shard, shardHosts := range shardHostsMap {
		group := clusterslogic.HostGroup{
			Role:                       hosts.RoleRedis,
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

	return len(shardHostsMap), result
}

func validateClusterNodes(ctx context.Context, session sessions.Session, validator clusterslogic.Validator, shardsNum int, pillar rpillars.Cluster, hostGroups ...clusterslogic.HostGroup) (clusterslogic.ResolvedHostGroups, error) {
	maxShardsNum := pillar.MaxShardsNum()
	minShardsNum := rmodels.MinShardsNum
	shardsNum64 := int64(shardsNum)
	if pillar.IsSharded() && shardsNum < 2 || !pillar.IsSharded() && shardsNum > 1 {
		// should never happen
		msg := fmt.Sprintf("number of shards %d doesn't match cluster sharding setting %t", shardsNum, pillar.IsSharded())
		return clusterslogic.ResolvedHostGroups{}, xerrors.New(msg)

	}
	if pillar.IsSharded() && (shardsNum64 > maxShardsNum || shardsNum64 < minShardsNum) {
		msg := fmt.Sprintf("number of shards must be between %d and %d.", minShardsNum, maxShardsNum)
		return clusterslogic.ResolvedHostGroups{}, xerrors.New(msg)
	}

	resolvedHostGroups, _, err := validator.ValidateResources(
		ctx,
		session,
		clusters.TypeRedis,
		hostGroups...,
	)
	return resolvedHostGroups, err
}

type creatorModifier interface {
	AddHosts(ctx context.Context, args []models.AddHostArgs) ([]hosts.Host, error)
	AddHostPillar(ctx context.Context, cid, fqdn string, revision int64, pillar pillars.Marshaler) error
	UpdateHostPillar(ctx context.Context, cid, fqdn string, revision int64, pillar pillars.Marshaler) error
	GenerateSemanticFQDNs(cloudType environment.CloudType, clusterType clusters.Type, zonesToCreate clusterslogic.ZoneHostsList,
		zonesCurrent clusterslogic.ZoneHostsList, shardName string, cid string, vtype environment.VType, platform compute.Platform) (map[string][]string, error)
}

type creatorReader interface {
	HostPillar(ctx context.Context, fqdn string, marshaller pillars.Marshaler) error
}

func (r *Redis) rClusterByCid(ctx context.Context, reader clusterslogic.Reader, cid string) (cluster, error) {
	c, err := reader.ClusterByClusterID(ctx, cid, clusters.TypeRedis, models.VisibilityVisible)
	if err != nil {
		return cluster{}, err
	}

	return r.rCluster(c)
}

func (r *Redis) rCluster(c clusterslogic.Cluster) (cluster, error) {
	var pillar rpillars.Cluster
	if err := c.Pillar(&pillar); err != nil {
		return cluster{}, err
	}

	return cluster{
		Cluster: c.Cluster,
		Pillar:  &pillar,
	}, nil
}

// createHosts creates hosts for single shard or subcluster
func (r *Redis) createHosts(
	ctx context.Context,
	session sessions.Session,
	reader creatorReader,
	modifier creatorModifier,
	cluster cluster,
	subCID string,
	shardID optional.String,
	shardName string,
	specs []rmodels.HostSpec,
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

	priorities := make(map[string]optional.Int64, len(specs))
	for _, spec := range specs {
		fqdn := fqdnMap[spec.ZoneID][0]
		fqdnMap[spec.ZoneID] = fqdnMap[spec.ZoneID][1:]
		priorities[fqdn] = spec.ReplicaPriority

		optSubnet := optional.NewString(spec.SubnetID)
		if len(spec.SubnetID) == 0 {
			optSubnet.Valid = false
		}

		subnet, err := r.compute.PickSubnet(ctx, subnets, resolvedGroup.TargetResourcePreset().VType, spec.ZoneID, spec.AssignPublicIP, optSubnet, session.FolderCoords.FolderExtID)
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

	addHosts, addErr := modifier.AddHosts(ctx, hostArgs)
	if addErr != nil {
		return nil, addErr
	}

	if !cluster.Pillar.IsSharded() {
		for _, host := range addHosts {
			fqdn := host.FQDN
			priority := priorities[fqdn]
			hostPillar := &rpillars.Host{}
			hostPillar.SetReplicaPriority(priority)
			err = modifier.AddHostPillar(ctx, cluster.ClusterID, fqdn, cluster.Revision, hostPillar)
			if err != nil {
				return nil, err
			}
		}
	}
	return addHosts, nil
}
