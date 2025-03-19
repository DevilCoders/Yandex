package provider

import (
	"context"
	"sort"
	"strings"
	"time"

	cheventspub "a.yandex-team.ru/cloud/bitbucket/public-api/yandex/cloud/events/mdb/clickhouse"
	networkProvider "a.yandex-team.ru/cloud/mdb/internal/network"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/clickhouse"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/clickhouse/chmodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/clickhouse/provider/internal/chpillars"
	clusterslogic "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/sessions"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/pagination"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/tasks"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/slices"
)

func getDefaultShardConfig(ctx context.Context, reader clusterslogic.Reader) (chmodels.ClickHouseConfig, error) {
	defaultSubClusterPillar := chpillars.NewSubClusterCH()
	err := reader.ClusterTypePillar(ctx, clusters.TypeClickHouse, defaultSubClusterPillar)
	if err != nil {
		return chmodels.ClickHouseConfig{}, err
	}

	defaultConfig, err := defaultSubClusterPillar.Data.ClickHouse.Config.ToModel()
	if err != nil {
		return chmodels.ClickHouseConfig{}, err
	}

	return defaultConfig, nil
}

func getCHShard(ctx context.Context, shard clusters.Shard, shardPillar chpillars.ShardCH, subCluster subCluster, defaultConfig chmodels.ClickHouseConfig, reader clusterslogic.Reader) (chmodels.Shard, error) {
	res := chmodels.Shard{
		Shard:     shard,
		ClusterID: subCluster.ClusterID,
	}

	// Set shard weight
	{
		s, found := subCluster.Pillar.Data.ClickHouse.Shards[shard.ShardID]
		if found {
			res.Config.Weight = s.Weight
		} else {
			res.Config.Weight = chmodels.DefaultShardWeight
		}
	}

	// Set shard config set
	{
		userConfig, err := shardPillar.Data.ClickHouse.Config.ToModel()
		if err != nil {
			return chmodels.Shard{}, err
		}
		effectiveConfig, err := chmodels.MergeClickhouseConfigs(defaultConfig, userConfig)
		if err != nil {
			return chmodels.Shard{}, err
		}

		res.Config.ConfigSet = chmodels.ClickhouseConfigSet{
			Default:   defaultConfig,
			User:      userConfig,
			Effective: effectiveConfig,
		}
	}

	// Set shard resources
	{
		shardHosts, err := reader.ListShardHosts(ctx, shard.ShardID, subCluster.ClusterID)
		if err != nil {
			return chmodels.Shard{}, err
		}

		res.Config.Resources = models.ClusterResources{
			ResourcePresetExtID: shardHosts[0].ResourcePresetExtID,
			DiskSize:            shardHosts[0].SpaceLimit,
			DiskTypeExtID:       shardHosts[0].DiskTypeExtID,
		}
	}

	return res, nil
}

func (ch *ClickHouse) GetShard(ctx context.Context, clusterID, shardName string) (chmodels.Shard, error) {
	var (
		res chmodels.Shard
		err error
	)

	err = ch.operator.ReadOnCluster(ctx, clusterID, clusters.TypeClickHouse,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, cluster clusterslogic.Cluster) error {
			var shardPillar chpillars.ShardCH
			shard, err := reader.ShardByShardName(ctx, shardName, clusterID, &shardPillar)
			if err != nil {
				return err
			}

			chSubCluster, err := ch.chSubCluster(ctx, reader, clusterID)
			if err != nil {
				return err
			}

			defaultConfig, err := getDefaultShardConfig(ctx, reader)
			if err != nil {
				return err
			}

			res, err = getCHShard(ctx, shard, shardPillar, chSubCluster, defaultConfig, reader)
			if err != nil {
				return err
			}

			return nil
		},
	)
	if err != nil {
		return chmodels.Shard{}, err
	}

	return res, nil
}

func (ch *ClickHouse) ListShards(ctx context.Context, clusterID string, pageSize int64, offset int64) ([]chmodels.Shard, pagination.OffsetPageToken, error) {
	var (
		res  []chmodels.Shard
		page pagination.Page
		err  error
	)

	err = ch.operator.ReadOnCluster(ctx, clusterID, clusters.TypeClickHouse,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, cluster clusterslogic.Cluster) error {
			metaShards, err := reader.ListShardsExtended(ctx, clusterID)
			if err != nil {
				return err
			}

			page = pagination.NewPage(int64(len(metaShards)), pageSize, offset)

			chSubCluster, err := ch.chSubCluster(ctx, reader, clusterID)
			if err != nil {
				return err
			}

			defaultConfig, err := getDefaultShardConfig(ctx, reader)
			if err != nil {
				return err
			}

			for i := page.LowerIndex; i < page.UpperIndex; i++ {
				var shardPillar chpillars.ShardCH
				err := shardPillar.UnmarshalPillar(metaShards[i].Pillar)
				if err != nil {
					return err
				}

				shard, err := getCHShard(ctx, metaShards[i].Shard, shardPillar, chSubCluster, defaultConfig, reader)
				if err != nil {
					return err
				}

				res = append(res, shard)
			}

			return nil
		},
	)
	if err != nil {
		return nil, pagination.OffsetPageToken{}, err
	}

	nextPageToken := pagination.OffsetPageToken{
		Offset: page.NextPageOffset,
		More:   page.HasMore,
	}

	return res, nextPageToken, nil
}

func (ch *ClickHouse) AddShard(ctx context.Context, cid string, args clickhouse.CreateShardArgs) (operations.Operation, error) {
	if err := chmodels.ValidateShardName(args.Name); err != nil {
		return operations.Operation{}, err
	}

	for _, spec := range args.HostSpecs {
		if spec.HostRole == hosts.RoleZooKeeper {
			return operations.Operation{}, semerr.InvalidInput("shard cannot have dedicated ZooKeeper hosts")
		}
	}

	return ch.operator.ModifyOnNotStoppedCluster(ctx, cid, clusters.TypeClickHouse, func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, modifier clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {
		currentHosts, err := clusterslogic.ListAllHosts(ctx, reader, cid)
		if err != nil {
			return operations.Operation{}, err
		}

		chHosts := clusterslogic.GetHostsWithRole(currentHosts, hosts.RoleClickHouse)
		zkHosts := clusterslogic.GetHostsWithRole(currentHosts, hosts.RoleZooKeeper)

		shards, err := reader.ListShards(ctx, cid)
		if err != nil {
			return operations.Operation{}, err
		}

		if int64(len(shards)+1) > ch.cfg.ClickHouse.ShardCountLimit && !session.FeatureFlags.Has(chmodels.ClickHouseUnlimitedShardCount) {
			return operations.Operation{}, semerr.FailedPreconditionf("cluster can have up to a maximum of %d ClickHouse shards", ch.cfg.ClickHouse.ShardCountLimit)
		}

		nameLower := strings.ToLower(args.Name)
		for _, shard := range shards {
			if strings.ToLower(shard.Name) == nameLower {
				return operations.Operation{}, semerr.AlreadyExistsf("cannot create shard %q: shard with name %q already exists", args.Name, shard.Name)
			}
		}

		oldestShardHosts := getOldestShardHosts(shards, chHosts)
		resources := models.ClusterResources{
			ResourcePresetExtID: oldestShardHosts[0].ResourcePresetExtID,
			DiskSize:            oldestShardHosts[0].SpaceLimit,
			DiskTypeExtID:       oldestShardHosts[0].DiskTypeExtID,
		}

		resources.Merge(args.ConfigSpec.Resources)

		if len(args.HostSpecs) == 0 {
			for _, host := range oldestShardHosts {
				args.HostSpecs = append(args.HostSpecs, chmodels.HostSpec{
					ZoneID:         host.ZoneID,
					HostRole:       hosts.RoleClickHouse,
					ShardName:      args.Name,
					AssignPublicIP: host.AssignPublicIP,
				})
			}
		}

		chCluster, err := ch.chCluster(ctx, reader, cid)
		if err != nil {
			return operations.Operation{}, err
		}

		chSubCluster, err := ch.chSubCluster(ctx, reader, cid)
		if err != nil {
			return operations.Operation{}, err
		}

		hasZk := len(zkHosts) > 0 || len(chSubCluster.Pillar.Data.ClickHouse.ZKHosts) > 0 || len(chSubCluster.Pillar.Data.ClickHouse.KeeperHosts) > 0

		if !hasZk && len(args.HostSpecs) > 1 {
			return operations.Operation{}, semerr.FailedPrecondition("shard cannot have more than 1 host in non-HA cluster configuration")
		}

		hostGroups := buildClusterHostGroups(chHosts, zkHosts, optional.String{},
			clusterslogic.ZoneHostsList{}, clusterslogic.ZoneHostsList{}, clusterslogic.ZoneHostsList{}, clusterslogic.ZoneHostsList{})
		hostGroups = append(hostGroups, clusterslogic.HostGroup{
			Role:                       hosts.RoleClickHouse,
			CurrentResourcePresetExtID: optional.NewString(resources.ResourcePresetExtID),
			DiskTypeExtID:              resources.DiskTypeExtID,
			CurrentDiskSize:            optional.NewInt64(resources.DiskSize),
			HostsToAdd:                 chmodels.ToZoneHosts(args.HostSpecs),
		})

		resolvedHostGroups, _, err := modifier.ValidateResources(ctx, session, clusters.TypeClickHouse, hostGroups...)
		if err != nil {
			return operations.Operation{}, err
		}

		if len(zkHosts) > 0 {
			if err := ch.validateZookeeperCores(ctx, session, modifier, resolvedHostGroups, "create an additional ClickHouse shard"); err != nil {
				return operations.Operation{}, err
			}
		}

		_, subnets, err := ch.compute.NetworkAndSubnets(ctx, cluster.NetworkID)
		if err != nil {
			return operations.Operation{}, err
		}

		shard, _, err := ch.createShard(ctx, session, modifier, chCluster, chSubCluster, resolvedHostGroups.SingleWithChanges(), subnets, args)
		if err != nil {
			return operations.Operation{}, err
		}

		if !args.ConfigSpec.Weight.Valid {
			args.ConfigSpec.Weight = optional.NewInt64(chmodels.DefaultShardWeight)
		}

		if err := chSubCluster.Pillar.AddShard(shard.ShardID, args.ConfigSpec.Weight.Must()); err != nil {
			return operations.Operation{}, err
		}

		for _, group := range args.ShardGroups {
			if err := chSubCluster.Pillar.AddShardInGroup(group, args.Name); err != nil {
				return operations.Operation{}, err
			}
		}

		if err := modifier.UpdateSubClusterPillar(ctx, cid, chSubCluster.SubClusterID, cluster.Revision, chSubCluster.Pillar); err != nil {
			return operations.Operation{}, err
		}

		op, err := ch.tasks.CreateTask(
			ctx,
			session,
			tasks.CreateTaskArgs{
				ClusterID:     cid,
				FolderID:      session.FolderCoords.FolderID,
				Auth:          session.Subject,
				OperationType: chmodels.OperationTypeShardCreate,
				TaskType:      chmodels.TaskTypeShardCreate,
				TaskArgs: map[string]interface{}{
					"shard_id":             shard.ShardID,
					"service_account_id":   chSubCluster.Pillar.Data.ServiceAccountID,
					"resetup_from_replica": args.CopySchema,
				},
				Timeout:  optional.NewDuration(time.Hour * 3),
				Metadata: chmodels.MetadataCreateShard{ShardName: args.Name},
				Revision: cluster.Revision,
			},
		)
		if err != nil {
			return operations.Operation{}, xerrors.Errorf("failed to create operation: %w", err)
		}

		event := &cheventspub.AddClusterShard{
			Authentication:  ch.events.NewAuthentication(session.Subject),
			Authorization:   ch.events.NewAuthorization(session.Subject),
			RequestMetadata: ch.events.NewRequestMetadata(ctx),
			EventStatus:     cheventspub.AddClusterShard_STARTED,
			Details: &cheventspub.AddClusterShard_EventDetails{
				ClusterId: cid,
				ShardName: args.Name,
			},
		}
		em, err := ch.events.NewEventMetadata(event, op, session.FolderCoords.CloudExtID, session.FolderCoords.FolderExtID)
		if err != nil {
			return operations.Operation{}, err
		}
		event.EventMetadata = em

		if err = ch.events.Store(ctx, event, op); err != nil {
			return operations.Operation{}, xerrors.Errorf("failed to save event for op %+v: %w", op, err)
		}

		return op, nil
	})
}

func getOldestShardHosts(shards []clusters.Shard, chHosts []hosts.HostExtended) []hosts.HostExtended {
	sort.Slice(shards, func(i, j int) bool {
		return shards[j].CreatedAt.After(shards[i].CreatedAt)
	})

	oldestShard := shards[0]
	var shardHosts []hosts.HostExtended
	for _, host := range chHosts {
		if host.ShardID.Must() == oldestShard.ShardID {
			shardHosts = append(shardHosts, host)
		}
	}

	return shardHosts
}

func (ch *ClickHouse) createShard(
	ctx context.Context,
	session sessions.Session,
	modifier creatorModifier,
	cluster cluster,
	sc subCluster,
	resolvedGroup clusterslogic.ResolvedHostGroup,
	subnets []networkProvider.Subnet,
	args clickhouse.CreateShardArgs,
) (clusters.Shard, []hosts.Host, error) {
	s, err := modifier.CreateShard(ctx, models.CreateShardArgs{
		ClusterID:    sc.ClusterID,
		SubClusterID: sc.SubClusterID,
		Name:         args.Name,
		Revision:     cluster.Revision,
	})
	if err != nil {
		return clusters.Shard{}, nil, err
	}

	newHosts, err := ch.createHosts(ctx, session, modifier, cluster, sc.SubClusterID, optional.NewString(s.ShardID), s.Name, args.HostSpecs, subnets, resolvedGroup)
	if err != nil {
		return clusters.Shard{}, nil, err
	}

	userConfigModel, err := sc.Pillar.Data.ClickHouse.Config.ToModel()
	if err != nil {
		return clusters.Shard{}, nil, err
	}
	shardConfigModel, err := chmodels.MergeClickhouseConfigs(userConfigModel, args.ConfigSpec.Config)
	if err != nil {
		return clusters.Shard{}, nil, err
	}

	shardPillar := &chpillars.SubClusterCH{}
	err = shardPillar.SetClickHouseConfig(ch.cryptoProvider, shardConfigModel)
	if err != nil {
		return clusters.Shard{}, nil, err
	}

	err = modifier.AddShardPillar(ctx, sc.ClusterID, s.ShardID, cluster.Revision, &chpillars.ShardCH{
		Data: chpillars.ShardCHData{
			ClickHouse: chpillars.ShardCHServer{
				Config: shardPillar.Data.ClickHouse.Config,
			},
		},
	})

	return s, newHosts, err
}

func (ch *ClickHouse) DeleteShard(ctx context.Context, cid, name string) (operations.Operation, error) {
	return ch.operator.DeleteOnCluster(ctx, cid, clusters.TypeClickHouse, func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, modifier clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {
		shards, err := reader.ListShards(ctx, cid)
		if err != nil {
			return operations.Operation{}, err
		}

		var targetShard clusters.Shard
		for _, shard := range shards {
			if shard.Name == name {
				targetShard = shard
				break
			}
		}
		if targetShard.Name != name {
			return operations.Operation{}, semerr.NotFoundf("shard %q does not exist", name)
		}

		if len(shards) == 1 {
			return operations.Operation{}, semerr.FailedPrecondition("last shard in cluster cannot be removed")
		}

		currentHosts, err := clusterslogic.ListAllHosts(ctx, reader, cluster.ClusterID)
		if err != nil {
			return operations.Operation{}, err
		}

		chHosts := clusterslogic.GetHostsWithRole(currentHosts, hosts.RoleClickHouse)
		zkHosts := clusterslogic.GetHostsWithRole(currentHosts, hosts.RoleZooKeeper)
		shardHosts := make([]hosts.HostExtended, 0, 3)
		for _, host := range chHosts {
			if host.ShardID.String == targetShard.ShardID {
				shardHosts = append(shardHosts, host)
			}
		}

		chSubCluster, err := ch.chSubCluster(ctx, reader, cid)
		if err != nil {
			return operations.Operation{}, err
		}

		if chSubCluster.Pillar.EmbeddedKeeper() {
			keeperHosts := chSubCluster.Pillar.Data.ClickHouse.ZKHosts
			for keeperHost := range chSubCluster.Pillar.Data.ClickHouse.KeeperHosts {
				keeperHosts = append(keeperHosts, keeperHost)
			}

			for _, shardHost := range shardHosts {
				if slices.ContainsString(keeperHosts, shardHost.FQDN) {
					return operations.Operation{},
						semerr.FailedPrecondition("Cluster reconfiguration affecting hosts with " +
							"ClickHouse Keeper is not currently supported.")

				}
			}
		}

		chSubCluster.Pillar.DeleteShard(targetShard.ShardID)

		if err := chSubCluster.Pillar.DeleteShardFromAllGroups(targetShard.Name); err != nil {
			return operations.Operation{}, err
		}

		if err := modifier.UpdateSubClusterPillar(ctx, cid, chSubCluster.SubClusterID, cluster.Revision, chSubCluster.Pillar); err != nil {
			return operations.Operation{}, err
		}

		_, _, err = modifier.ValidateResources(
			ctx,
			session,
			clusters.TypeClickHouse,
			buildClusterHostGroups(chHosts, zkHosts, optional.NewString(targetShard.ShardID), nil,
				clusterslogic.ZoneHostsListFromHosts(shardHosts), nil, nil)...,
		)
		if err != nil {
			return operations.Operation{}, err
		}

		if err := modifier.DeleteShard(ctx, cid, targetShard.ShardID, cluster.Revision); err != nil {
			return operations.Operation{}, err
		}

		hostArgs := deletedHostsToArgs(hosts.HostsExtendedToHosts(shardHosts), "")
		op, err := ch.tasks.CreateTask(
			ctx,
			session,
			tasks.CreateTaskArgs{
				ClusterID:     cid,
				FolderID:      session.FolderCoords.FolderID,
				Auth:          session.Subject,
				OperationType: chmodels.OperationTypeShardDelete,
				TaskType:      chmodels.TaskTypeShardDelete,
				TaskArgs: map[string]interface{}{
					"shard_hosts": hostArgs,
					"zk_hosts":    getZkHostTaskArg(zkHosts),
				},
				Metadata: chmodels.MetadataDeleteShard{ShardName: name},
				Revision: cluster.Revision,
				Timeout:  optional.NewDuration(time.Hour * 10),
			},
		)
		if err != nil {
			return operations.Operation{}, xerrors.Errorf("failed to create operation: %w", err)
		}

		event := &cheventspub.DeleteClusterShard{
			Authentication:  ch.events.NewAuthentication(session.Subject),
			Authorization:   ch.events.NewAuthorization(session.Subject),
			RequestMetadata: ch.events.NewRequestMetadata(ctx),
			EventStatus:     cheventspub.DeleteClusterShard_STARTED,
			Details: &cheventspub.DeleteClusterShard_EventDetails{
				ClusterId: cid,
				ShardName: name,
			},
		}
		em, err := ch.events.NewEventMetadata(event, op, session.FolderCoords.CloudExtID, session.FolderCoords.FolderExtID)
		if err != nil {
			return operations.Operation{}, err
		}
		event.EventMetadata = em

		if err = ch.events.Store(ctx, event, op); err != nil {
			return operations.Operation{}, xerrors.Errorf("failed to save event for op %+v: %w", op, err)
		}

		return op, nil
	})
}

// generateNewShardNames takes current shard names and generates new names which indices are greater than existing.
func generateNewShardNames(curShardNames []string, count int64) ([]string, error) {
	var curIndices []int
	for _, shardName := range curShardNames {
		ind, err := chmodels.ParseShardName(shardName)
		if err != nil {
			return nil, err
		}
		curIndices = append(curIndices, ind)
	}

	sort.Ints(curIndices)

	var lstIndex int
	if len(curIndices) < 1 {
		lstIndex = 0
	} else {
		lstIndex = curIndices[len(curIndices)-1]
	}

	var newIndices []int
	for i := range make([]int, count) {
		newIndices = append(newIndices, lstIndex+i+1)
	}

	var res []string
	for _, i := range newIndices {
		res = append(res, chmodels.GenerateShardName(i))
	}

	return res, nil
}

type ShardSpec struct {
	ShardName string
	HostSpecs []chmodels.HostSpec
}

func (ch ClickHouse) planAddClickHouseShards(
	session sessions.Session,
	clusterResources chmodels.DataCloudResources,
	hostGroupMap map[string]*clusterslogic.HostGroup,
	shards []clusters.Shard,
) ([]ShardSpec, error) {
	if !clusterResources.ShardCount.Valid {
		return nil, nil
	}

	requestedShardCount := clusterResources.ShardCount.Must()
	if requestedShardCount <= int64(len(hostGroupMap)) {
		return nil, nil
	}

	if requestedShardCount > ch.cfg.ClickHouse.ShardCountLimit && !session.FeatureFlags.Has(chmodels.ClickHouseUnlimitedShardCount) {
		return nil, semerr.FailedPreconditionf("cluster can have up to a maximum of %d ClickHouse shards", ch.cfg.ClickHouse.ShardCountLimit)
	}

	var currentShardNames []string
	for _, shard := range shards {
		currentShardNames = append(currentShardNames, shard.Name)
	}

	var sampleHostGroup *clusterslogic.HostGroup
	for _, group := range hostGroupMap {
		sampleHostGroup = group
	}
	newShardNames, err := generateNewShardNames(currentShardNames, requestedShardCount-int64(len(hostGroupMap)))
	if err != nil {
		return nil, err
	}

	var res []ShardSpec
	for _, newShardName := range newShardNames {
		var hostSpecs []chmodels.HostSpec

		zoneHosts := sampleHostGroup.HostsCurrent
		zoneHosts = zoneHosts.Add(sampleHostGroup.HostsToAdd)
		zoneHosts, err = zoneHosts.Sub(sampleHostGroup.HostsToRemove)
		if err != nil {
			return nil, err
		}

		for _, zoneHost := range zoneHosts {
			for range make([]int, zoneHost.Count) {
				hostSpecs = append(hostSpecs, chmodels.HostSpec{
					ZoneID:         zoneHost.ZoneID,
					HostRole:       sampleHostGroup.Role,
					ShardName:      newShardName,
					AssignPublicIP: true,
				})
			}
		}

		hostGroup := clusterslogic.HostGroup{
			Role:                       sampleHostGroup.Role,
			CurrentResourcePresetExtID: sampleHostGroup.CurrentResourcePresetExtID,
			NewResourcePresetExtID:     sampleHostGroup.NewResourcePresetExtID,
			DiskTypeExtID:              sampleHostGroup.DiskTypeExtID,
			CurrentDiskSize:            sampleHostGroup.CurrentDiskSize,
			NewDiskSize:                sampleHostGroup.NewDiskSize,
			HostsToAdd:                 chmodels.ToZoneHosts(hostSpecs),
			ShardName:                  optional.NewString(newShardName),
		}

		res = append(res,
			ShardSpec{
				ShardName: newShardName,
				HostSpecs: hostSpecs,
			})

		hostGroupMap[newShardName] = &hostGroup
	}

	return res, err
}

func (ch ClickHouse) addShards(
	ctx context.Context,
	session sessions.Session,
	modifier clusterslogic.Modifier,
	hostGroupMap map[string]clusterslogic.ResolvedHostGroup,
	cluster cluster,
	chSubCluster subCluster,
	shardSpecs []ShardSpec,
) ([]hosts.Host, error) {
	var newHosts []hosts.Host

	chConfig, err := chSubCluster.Pillar.Data.ClickHouse.Config.ToModel()
	if err != nil {
		return nil, err
	}

	for _, shardSpec := range shardSpecs {
		hostGroup := hostGroupMap[shardSpec.ShardName]
		newShard, newShardHosts, err := ch.createShard(
			ctx,
			session,
			modifier,
			cluster,
			chSubCluster,
			hostGroup,
			nil,
			clickhouse.CreateShardArgs{
				Name: shardSpec.ShardName,
				ConfigSpec: clickhouse.ShardConfigSpec{
					Config: chConfig,
					Resources: models.ClusterResourcesSpec{
						ResourcePresetExtID: optional.NewString(hostGroup.TargetResourcePresetExtID()),
						DiskSize:            optional.NewInt64(hostGroup.TargetDiskSize()),
						DiskTypeExtID:       optional.NewString(hostGroup.DiskTypeExtID),
					},
					Weight: optional.NewInt64(chmodels.DefaultShardWeight),
				},
				HostSpecs:  shardSpec.HostSpecs,
				CopySchema: true,
			},
		)
		if err != nil {
			return nil, err
		}

		if err = chSubCluster.Pillar.AddShard(newShard.ShardID, chmodels.DefaultShardWeight); err != nil {
			return nil, err
		}

		newHosts = append(newHosts, newShardHosts...)
	}

	return newHosts, nil
}

func (ch ClickHouse) planDeleteClickHouseShards(
	clusterResources chmodels.DataCloudResources,
	hostGroupMap map[string]*clusterslogic.HostGroup,
	shards []clusters.Shard,
) ([]clusters.Shard, error) {
	if !clusterResources.ShardCount.Valid {
		return nil, nil
	}

	requestedShardCount := int(clusterResources.ShardCount.Must())
	if requestedShardCount >= len(hostGroupMap) {
		return nil, nil
	}

	if requestedShardCount <= 0 {
		return nil, semerr.InvalidInputf("invalid shard count '%d', cluster must have at least one shard", requestedShardCount)
	}

	chmodels.SortShardsByName(shards)

	var toDelete []clusters.Shard
	for _, shard := range shards[requestedShardCount:] {
		toDelete = append(toDelete, shard)
		hostGroupMap[shard.ShardID].HostsToRemove = hostGroupMap[shard.ShardID].HostsCurrent
	}

	return toDelete, nil
}

func deleteShard(
	ctx context.Context,
	modifier clusterslogic.Modifier,
	cluster cluster,
	chSubCluster subCluster,
	shard clusters.Shard,
) error {
	chSubCluster.Pillar.DeleteShard(shard.ShardID)

	if err := chSubCluster.Pillar.DeleteShardFromAllGroups(shard.Name); err != nil {
		return err
	}

	if err := modifier.DeleteShard(ctx, cluster.ClusterID, shard.ShardID, cluster.Revision); err != nil {
		return err
	}

	return nil
}
