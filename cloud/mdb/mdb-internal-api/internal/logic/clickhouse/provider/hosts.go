package provider

import (
	"context"
	"math/rand"
	"sort"
	"time"

	cheventspub "a.yandex-team.ru/cloud/bitbucket/public-api/yandex/cloud/events/mdb/clickhouse"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/clickhouse/chmodels"
	clusterslogic "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/sessions"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/environment"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/pagination"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/tasks"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/slices"
)

func (ch *ClickHouse) ListHosts(ctx context.Context, cid string, pageSize, offset int64) ([]hosts.HostExtended, pagination.OffsetPageToken, error) {
	var (
		hostList  []hosts.HostExtended
		newOffset int64
		more      bool
	)

	if err := ch.operator.ReadOnCluster(ctx, cid, clusters.TypeClickHouse,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, _ clusterslogic.Cluster) error {
			var err error
			hostList, newOffset, more, err = reader.ListHosts(ctx, cid, pageSize, offset)
			if err != nil {
				return err
			}

			shards, err := reader.ListShards(ctx, cid)
			if err != nil {
				return err
			}

			shardMap := map[string]string{}
			for _, shard := range shards {
				shardMap[shard.ShardID] = shard.Name
			}

			for i, host := range hostList {
				if name, ok := shardMap[host.ShardID.String]; ok {
					hostList[i].ShardID.Set(name)
				}
			}

			return nil
		},
	); err != nil {
		return nil, pagination.OffsetPageToken{Offset: 0}, err
	}

	return hostList, pagination.OffsetPageToken{
		Offset: newOffset,
		More:   more,
	}, nil
}

func (ch *ClickHouse) AddHosts(ctx context.Context, cid string, specs []chmodels.HostSpec, copySchema bool) (operations.Operation, error) {
	if len(specs) > 1 {
		return operations.Operation{}, semerr.NotImplemented("adding multiple hosts at once is not supported yet")
	}

	if len(specs) < 1 {
		return operations.Operation{}, semerr.InvalidInput("no hosts to add are specified")
	}

	spec := specs[0]
	switch spec.HostRole {
	case hosts.RoleClickHouse:
		return ch.AddClickHouseHost(ctx, cid, spec, copySchema)
	case hosts.RoleZooKeeper:
		return ch.AddZookeeperHost(ctx, cid, spec)
	default:
		return operations.Operation{}, semerr.InvalidInputf("invalid host type %q", spec.HostRole.String())
	}
}

func (ch *ClickHouse) AddClickHouseHost(ctx context.Context, cid string, spec chmodels.HostSpec, copySchema bool) (operations.Operation, error) {
	return ch.operator.ModifyOnNotStoppedCluster(ctx, cid, clusters.TypeClickHouse,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, modifier clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {
			shards, err := reader.ListShards(ctx, cid)
			if err != nil {
				return operations.Operation{}, err
			}

			var targetShard clusters.Shard

			if spec.ShardName == "" {
				if len(shards) > 1 {
					return operations.Operation{}, semerr.FailedPrecondition("shard name must be specified in order to add host to sharded cluster")
				}

				targetShard = shards[0]
			} else {
				found := false
				for _, s := range shards {
					if s.Name == spec.ShardName {
						targetShard = s
						found = true
						break
					}
				}

				if !found {
					return operations.Operation{}, semerr.NotFoundf("shard %q does not exist", spec.ShardName)
				}
			}

			currentHosts, err := clusterslogic.ListAllHosts(ctx, reader, cluster.ClusterID)
			if err != nil {
				return operations.Operation{}, err
			}

			chHosts := clusterslogic.GetHostsWithRole(currentHosts, hosts.RoleClickHouse)
			zkHosts := clusterslogic.GetHostsWithRole(currentHosts, hosts.RoleZooKeeper)
			shardHosts := make([]hosts.HostExtended, 0, len(chHosts))
			for _, h := range chHosts {
				if h.ShardID.Must() == targetShard.ShardID {
					shardHosts = append(shardHosts, h)
				}
			}

			if len(shardHosts) == 0 {
				return operations.Operation{}, semerr.FailedPrecondition("there no hosts in shard")
			}

			sc, err := ch.chSubCluster(ctx, reader, cid)
			if err != nil {
				return operations.Operation{}, err
			}

			if len(shardHosts) == 1 && len(zkHosts) == 0 && len(sc.Pillar.Data.ClickHouse.ZKHosts) == 0 && len(sc.Pillar.Data.ClickHouse.KeeperHosts) == 0 {
				if len(shardHosts) == len(chHosts) {
					return operations.Operation{}, semerr.FailedPrecondition("cluster cannot have more than 1 host in non-HA configuration")
				}

				return operations.Operation{}, semerr.FailedPrecondition("shard cannot have more than 1 host in non-HA cluster configuration")
			}

			resolvedHostGroups, _, err := modifier.ValidateResources(
				ctx,
				session,
				clusters.TypeClickHouse,
				buildClusterHostGroups(chHosts, zkHosts, optional.NewString(targetShard.ShardID),
					clusterslogic.ZoneHostsList{clusterslogic.ZoneHosts{ZoneID: spec.ZoneID, Count: 1}}, nil, nil, nil)...,
			)
			if err != nil {
				return operations.Operation{}, err
			}

			if err := ch.validateZookeeperCores(ctx, session, modifier, resolvedHostGroups, "create additional ClickHouse hosts"); err != nil {
				return operations.Operation{}, err
			}

			_, subnets, err := ch.compute.NetworkAndSubnets(ctx, cluster.NetworkID)
			if err != nil {
				return operations.Operation{}, err
			}

			chCluster, err := ch.chCluster(ctx, reader, cid)
			if err != nil {
				return operations.Operation{}, err
			}

			createdHosts, err := ch.createHosts(
				ctx,
				session,
				modifier,
				chCluster,
				targetShard.SubClusterID,
				optional.NewString(targetShard.ShardID),
				targetShard.Name,
				[]chmodels.HostSpec{spec},
				subnets,
				resolvedHostGroups.SingleWithChanges(),
			)
			if err != nil {
				return operations.Operation{}, err
			}

			timeout := time.Hour * 3
			if copySchema {
				timeout += time.Hour * 24 * 3
			}

			op, err := ch.tasks.CreateTask(
				ctx,
				session,
				tasks.CreateTaskArgs{
					ClusterID:     cid,
					FolderID:      session.FolderCoords.FolderID,
					Auth:          session.Subject,
					TaskType:      chmodels.TaskTypeHostCreate,
					OperationType: chmodels.OperationTypeHostCreate,
					Revision:      cluster.Revision,
					Timeout:       optional.NewDuration(timeout),
					Metadata: chmodels.MetadataCreateHosts{
						HostNames: []string{createdHosts[0].FQDN},
					},
					TaskArgs: map[string]interface{}{
						"host":                 createdHosts[0].FQDN,
						"subcid":               createdHosts[0].SubClusterID,
						"shard_id":             createdHosts[0].ShardID.Must(),
						"service_account_id":   sc.Pillar.Data.ServiceAccountID,
						"resetup_from_replica": copySchema,
					},
				})
			if err != nil {
				return operations.Operation{}, xerrors.Errorf("failed to create operation: %w", err)
			}

			if err := ch.createAddHostsEvent(ctx, session, op, cid, createdHosts[0].FQDN); err != nil {
				return operations.Operation{}, err
			}

			return op, nil
		})
}

func (ch *ClickHouse) AddZookeeperHost(ctx context.Context, cid string, spec chmodels.HostSpec) (operations.Operation, error) {
	return ch.operator.ModifyOnNotStoppedCluster(ctx, cid, clusters.TypeClickHouse,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, modifier clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {
			currentHosts, err := clusterslogic.ListAllHosts(ctx, reader, cluster.ClusterID)
			if err != nil {
				return operations.Operation{}, err
			}

			chHosts := clusterslogic.GetHostsWithRole(currentHosts, hosts.RoleClickHouse)
			zkHosts := clusterslogic.GetHostsWithRole(currentHosts, hosts.RoleZooKeeper)
			if len(zkHosts) == 0 {
				return operations.Operation{}, semerr.FailedPrecondition("cluster does not have ZooKeeper")
			}

			resolvedHostGroups, _, err := modifier.ValidateResources(
				ctx,
				session,
				clusters.TypeClickHouse,
				buildClusterHostGroups(chHosts, zkHosts, optional.String{}, nil, nil,
					clusterslogic.ZoneHostsList{clusterslogic.ZoneHosts{
						ZoneID: spec.ZoneID,
						Count:  1,
					}}, nil)...,
			)
			if err != nil {
				return operations.Operation{}, err
			}

			resolvedHostGroup := resolvedHostGroups.MustGroupsByHostRole(hosts.RoleZooKeeper)[0]

			_, subnets, err := ch.compute.NetworkAndSubnets(ctx, cluster.NetworkID)
			if err != nil {
				return operations.Operation{}, err
			}

			chCluster, err := ch.chCluster(ctx, reader, cid)
			if err != nil {
				return operations.Operation{}, err
			}

			createdHosts, err := ch.createHosts(
				ctx,
				session,
				modifier,
				chCluster,
				zkHosts[0].SubClusterID,
				optional.String{},
				chmodels.ZooKeeperShardName,
				[]chmodels.HostSpec{spec},
				subnets,
				resolvedHostGroup,
			)
			if err != nil {
				return operations.Operation{}, err
			}

			sz, err := ch.zkSubCluster(ctx, reader, cid)
			if err != nil {
				return operations.Operation{}, err
			}

			zid, err := sz.Pillar.AddNode(createdHosts[0].FQDN)
			if err != nil {
				return operations.Operation{}, err
			}

			if err = modifier.UpdateSubClusterPillar(ctx, sz.ClusterID, sz.SubClusterID, cluster.Revision, sz.Pillar); err != nil {
				return operations.Operation{}, err
			}

			op, err := ch.tasks.CreateTask(
				ctx,
				session,
				tasks.CreateTaskArgs{
					ClusterID:     cid,
					FolderID:      session.FolderCoords.FolderID,
					Auth:          session.Subject,
					TaskType:      chmodels.TaskTypeZookeeperHostCreate,
					OperationType: chmodels.OperationTypeHostCreate,
					Revision:      cluster.Revision,
					Metadata: chmodels.MetadataCreateHosts{
						HostNames: []string{createdHosts[0].FQDN},
					},
					TaskArgs: map[string]interface{}{
						"host":      createdHosts[0].FQDN,
						"subcid":    createdHosts[0].SubClusterID,
						"zk_hosts":  getZkHostTaskArg(zkHosts),
						"zid_added": zid,
					},
				})
			if err != nil {
				return operations.Operation{}, xerrors.Errorf("failed to create operation: %w", err)
			}

			if err := ch.createAddHostsEvent(ctx, session, op, cid, createdHosts[0].FQDN); err != nil {
				return operations.Operation{}, err
			}

			return op, nil
		})
}

func (ch *ClickHouse) createAddHostsEvent(ctx context.Context, session sessions.Session, op operations.Operation, cid, hostName string) error {
	event := &cheventspub.AddClusterHosts{
		Authentication:  ch.events.NewAuthentication(session.Subject),
		Authorization:   ch.events.NewAuthorization(session.Subject),
		RequestMetadata: ch.events.NewRequestMetadata(ctx),
		EventStatus:     cheventspub.AddClusterHosts_STARTED,
		Details: &cheventspub.AddClusterHosts_EventDetails{
			ClusterId: cid,
			HostNames: []string{hostName},
		},
	}
	em, err := ch.events.NewEventMetadata(event, op, session.FolderCoords.CloudExtID, session.FolderCoords.FolderExtID)
	if err != nil {
		return err
	}
	event.EventMetadata = em

	if err = ch.events.Store(ctx, event, op); err != nil {
		return xerrors.Errorf("failed to save event for op %+v: %w", op, err)
	}

	return nil
}

func (ch *ClickHouse) UpdateHosts(ctx context.Context, cid string, specs []chmodels.UpdateHostSpec) (operations.Operation, error) {
	if len(specs) > 1 {
		return operations.Operation{}, semerr.NotImplemented("updating multiple hosts at once is not supported yet")
	}

	if len(specs) < 1 {
		return operations.Operation{}, semerr.InvalidInput("no hosts to update are specified")
	}

	spec := specs[0]
	return ch.UpdateClickHouseHost(ctx, cid, spec)
}

func (ch *ClickHouse) UpdateClickHouseHost(ctx context.Context, cid string, spec chmodels.UpdateHostSpec) (operations.Operation, error) {
	return ch.operator.ModifyOnNotStoppedCluster(ctx, cid, clusters.TypeClickHouse,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, modifier clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {
			currentHosts, err := clusterslogic.ListAllHosts(ctx, reader, cluster.ClusterID)
			if err != nil {
				return operations.Operation{}, err
			}

			chHosts := clusterslogic.GetHostsWithRole(currentHosts, hosts.RoleClickHouse)

			var host hosts.HostExtended
			var found = false
			for _, h := range chHosts {
				if h.FQDN == spec.HostName {
					host = h
					found = true
					break
				}
			}

			if !found {
				zkHosts := clusterslogic.GetHostsWithRole(currentHosts, hosts.RoleZooKeeper)
				for _, h := range zkHosts {
					if h.FQDN == spec.HostName {
						return operations.Operation{}, xerrors.Errorf("trying to assing public ip to zookeeper host: %s", spec.HostName)
					}
				}
				return operations.Operation{}, xerrors.Errorf("there is no host with such FQDN: %s", spec.HostName)
			}

			needModifyAssignPublicIP := spec.AssignPublicIP.Valid && spec.AssignPublicIP.Bool != host.AssignPublicIP

			if needModifyAssignPublicIP {
				vtype, err := environment.ParseVType(host.VType)
				if err != nil {
					return operations.Operation{}, err
				}
				if vtype == environment.VTypePorto {
					return operations.Operation{}, xerrors.Errorf("public ip assignment is not supported in internal MDB")
				}
				_, subnets, err := ch.compute.NetworkAndSubnets(ctx, cluster.NetworkID)
				if err != nil {
					return operations.Operation{}, err
				}
				subnet, err := ch.compute.PickSubnet(ctx, subnets, vtype, host.ZoneID, spec.AssignPublicIP.Bool, optional.NewString(host.SubnetID), session.FolderCoords.FolderExtID)
				if err != nil {
					return operations.Operation{}, err
				}
				if subnet.V4CIDRBlocks == nil {
					return operations.Operation{}, xerrors.Errorf("public ip assignment in ipv6-only networks is not supported")
				}

				err = modifier.ModifyHostPublicIP(ctx, cluster.ClusterID, spec.HostName, cluster.Revision, spec.AssignPublicIP.Bool)
				if err != nil {
					return operations.Operation{}, err
				}
			}

			timeout := time.Minute * 3

			hostArgs := make(map[string]interface{})
			taskArgs := make(map[string]interface{})
			hostArgs["fqdn"] = spec.HostName
			taskArgs["host"] = hostArgs
			if needModifyAssignPublicIP {
				taskArgs["include-metadata"] = spec.AssignPublicIP.Bool
			}

			op, err := ch.tasks.CreateTask(
				ctx,
				session,
				tasks.CreateTaskArgs{
					ClusterID:     cid,
					FolderID:      session.FolderCoords.FolderID,
					Auth:          session.Subject,
					TaskType:      chmodels.TaskTypeHostUpdate,
					OperationType: chmodels.OperationTypeHostUpdate,
					Revision:      cluster.Revision,
					Timeout:       optional.NewDuration(timeout),
					Metadata: chmodels.MetadataUpdateHosts{
						HostNames: []string{spec.HostName},
					},
					TaskArgs: taskArgs,
				})
			if err != nil {
				return operations.Operation{}, xerrors.Errorf("failed to create operation: %w", err)
			}

			if err := ch.createUpdateHostsEvent(ctx, session, op, cid, spec.HostName); err != nil {
				return operations.Operation{}, err
			}

			return op, nil
		})
}

func (ch *ClickHouse) createUpdateHostsEvent(ctx context.Context, session sessions.Session, op operations.Operation, cid, hostName string) error {
	event := &cheventspub.UpdateClusterHosts{
		Authentication:  ch.events.NewAuthentication(session.Subject),
		Authorization:   ch.events.NewAuthorization(session.Subject),
		RequestMetadata: ch.events.NewRequestMetadata(ctx),
		EventStatus:     cheventspub.UpdateClusterHosts_STARTED,
		Details: &cheventspub.UpdateClusterHosts_EventDetails{
			ClusterId: cid,
			HostNames: []string{hostName},
		},
	}
	em, err := ch.events.NewEventMetadata(event, op, session.FolderCoords.CloudExtID, session.FolderCoords.FolderExtID)
	if err != nil {
		return err
	}
	event.EventMetadata = em

	if err = ch.events.Store(ctx, event, op); err != nil {
		return xerrors.Errorf("failed to save event for op %+v: %w", op, err)
	}

	return nil
}

func (ch *ClickHouse) DeleteHosts(ctx context.Context, cid string, fqdns []string) (operations.Operation, error) {
	return ch.operator.ModifyOnNotStoppedCluster(ctx, cid, clusters.TypeClickHouse,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, modifier clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {
			if len(fqdns) > 1 {
				return operations.Operation{}, semerr.NotImplemented("deleting multiple hosts at once is not supported yet")
			}

			if len(fqdns) < 1 {
				return operations.Operation{}, semerr.FailedPrecondition("no hosts to delete are specified")
			}
			fqdnToDelete := fqdns[0]

			chSubCluster, err := ch.chSubCluster(ctx, reader, cid)
			if err != nil {
				return operations.Operation{}, err
			}

			if chSubCluster.Pillar.EmbeddedKeeper() {
				keeperHosts := chSubCluster.Pillar.Data.ClickHouse.ZKHosts
				for keeperHost := range chSubCluster.Pillar.Data.ClickHouse.KeeperHosts {
					keeperHosts = append(keeperHosts, keeperHost)
				}

				if slices.ContainsString(keeperHosts, fqdnToDelete) {
					return operations.Operation{},
						semerr.FailedPrecondition("Cluster reconfiguration affecting hosts with " +
							"ClickHouse Keeper is not currently supported.")

				}
			}

			currentHosts, err := clusterslogic.ListAllHosts(ctx, reader, cluster.ClusterID)
			if err != nil {
				return operations.Operation{}, err
			}

			chHosts := clusterslogic.GetHostsWithRole(currentHosts, hosts.RoleClickHouse)
			zkHosts := clusterslogic.GetHostsWithRole(currentHosts, hosts.RoleZooKeeper)

			hostToDelete, err := clusterslogic.HostExtendedByFQDN(zkHosts, fqdnToDelete)
			if err == nil {
				return ch.deleteZooKeeperHost(ctx, session, reader, modifier, cluster, hostToDelete, chHosts, zkHosts)
			}

			hostToDelete, err = clusterslogic.HostExtendedByFQDN(chHosts, fqdnToDelete)
			if err == nil {
				return ch.deleteClickHouseHost(ctx, session, modifier, cluster, hostToDelete, chHosts, zkHosts)
			}

			return operations.Operation{}, err
		})
}

func (ch *ClickHouse) deleteClickHouseHost(ctx context.Context, session sessions.Session, modifier clusterslogic.Modifier,
	cluster clusterslogic.Cluster, host hosts.HostExtended, chHosts, zkHosts []hosts.HostExtended) (operations.Operation, error) {
	shardHosts := make([]hosts.HostExtended, 0, len(chHosts))
	for _, h := range chHosts {
		if h.ShardID.Must() == host.ShardID.Must() {
			shardHosts = append(shardHosts, h)
		}
	}

	if len(shardHosts) <= 1 {
		if len(shardHosts) == len(chHosts) {
			return operations.Operation{}, semerr.FailedPrecondition("last host in cluster cannot be removed")
		}

		return operations.Operation{}, semerr.FailedPrecondition("last host in shard cannot be removed")
	}

	if !session.FeatureFlags.Has(chmodels.ClickHouseDisableClusterConfigurationChecks) {
		if len(shardHosts) == 2 {
			if len(shardHosts) == len(chHosts) {
				return operations.Operation{}, semerr.FailedPrecondition("cluster cannot have less than 2 ClickHouse hosts in HA configuration")
			}

			return operations.Operation{}, semerr.FailedPrecondition("shard cannot have less than 2 ClickHouse hosts in HA cluster configuration")
		}
	}

	_, _, err := modifier.ValidateResources(
		ctx,
		session,
		clusters.TypeClickHouse,
		buildClusterHostGroups(chHosts, zkHosts, host.ShardID, nil,
			clusterslogic.ZoneHostsList{clusterslogic.ZoneHosts{
				ZoneID: host.ZoneID,
				Count:  1,
			}}, nil, nil)...,
	)
	if err != nil {
		return operations.Operation{}, err
	}

	deletedHosts, err := modifier.DeleteHosts(ctx, cluster.ClusterID, []string{host.FQDN}, cluster.Revision)
	if err != nil {
		return operations.Operation{}, err

	}

	op, err := ch.tasks.CreateTask(
		ctx,
		session,
		tasks.CreateTaskArgs{
			ClusterID:     cluster.ClusterID,
			FolderID:      session.FolderCoords.FolderID,
			Auth:          session.Subject,
			TaskType:      chmodels.TaskTypeHostDelete,
			OperationType: chmodels.OperationTypeHostDelete,
			Revision:      cluster.Revision,
			Timeout:       optional.NewDuration(time.Hour * 10),
			Metadata: chmodels.MetadataDeleteHosts{
				HostNames: []string{deletedHosts[0].FQDN},
			},
			TaskArgs: map[string]interface{}{
				"host": map[string]interface{}{
					"fqdn":     deletedHosts[0].FQDN,
					"subcid":   deletedHosts[0].SubClusterID,
					"vtype":    deletedHosts[0].VType,
					"vtype_id": deletedHosts[0].VTypeID.String,
					"shard_id": deletedHosts[0].ShardID.String,
				},
				"zk_hosts": getZkHostTaskArg(zkHosts),
			},
		})
	if err != nil {
		return operations.Operation{}, xerrors.Errorf("failed to create operation: %w", err)
	}

	if err = ch.createDeleteHostsEvent(ctx, session, op, cluster.ClusterID, host.FQDN); err != nil {
		return operations.Operation{}, err
	}

	return op, nil
}

func (ch *ClickHouse) deleteZooKeeperHost(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, modifier clusterslogic.Modifier,
	cluster clusterslogic.Cluster, host hosts.HostExtended, chHosts, zkHosts []hosts.HostExtended) (operations.Operation, error) {
	_, _, err := modifier.ValidateResources(
		ctx,
		session,
		clusters.TypeClickHouse,
		buildClusterHostGroups(chHosts, zkHosts, optional.String{}, nil, nil, nil,
			clusterslogic.ZoneHostsList{clusterslogic.ZoneHosts{
				ZoneID: host.ZoneID,
				Count:  1,
			}})...,
	)
	if err != nil {
		return operations.Operation{}, err
	}

	sz, err := ch.zkSubCluster(ctx, reader, cluster.ClusterID)
	if err != nil {
		return operations.Operation{}, err
	}

	zid, err := sz.Pillar.DeleteNode(host.FQDN)
	if err != nil {
		return operations.Operation{}, err
	}

	if err = modifier.UpdateSubClusterPillar(ctx, sz.ClusterID, sz.SubClusterID, cluster.Revision, sz.Pillar); err != nil {
		return operations.Operation{}, err
	}

	deletedHosts, err := modifier.DeleteHosts(ctx, cluster.ClusterID, []string{host.FQDN}, cluster.Revision)
	if err != nil {
		return operations.Operation{}, err

	}

	op, err := ch.tasks.CreateTask(
		ctx,
		session,
		tasks.CreateTaskArgs{
			ClusterID:     cluster.ClusterID,
			FolderID:      session.FolderCoords.FolderID,
			Auth:          session.Subject,
			TaskType:      chmodels.TaskTypeZookeeperHostDelete,
			OperationType: chmodels.OperationTypeHostDelete,
			Revision:      cluster.Revision,
			Timeout:       optional.NewDuration(time.Hour * 10),
			Metadata: chmodels.MetadataDeleteHosts{
				HostNames: []string{deletedHosts[0].FQDN},
			},
			TaskArgs: map[string]interface{}{
				"host": map[string]interface{}{
					"fqdn":     deletedHosts[0].FQDN,
					"subcid":   deletedHosts[0].SubClusterID,
					"vtype":    deletedHosts[0].VType,
					"vtype_id": deletedHosts[0].VTypeID.String,
				},
				// Deleted host still used in connection string
				"zk_hosts":    getZkHostTaskArg(zkHosts),
				"zid_deleted": zid,
			},
		})
	if err != nil {
		return operations.Operation{}, xerrors.Errorf("failed to create operation: %w", err)
	}

	if err = ch.createDeleteHostsEvent(ctx, session, op, cluster.ClusterID, host.FQDN); err != nil {
		return operations.Operation{}, err
	}

	return op, nil
}

func (ch *ClickHouse) createDeleteHostsEvent(ctx context.Context, session sessions.Session, op operations.Operation, cid, hostName string) error {
	event := &cheventspub.DeleteClusterHosts{
		Authentication:  ch.events.NewAuthentication(session.Subject),
		Authorization:   ch.events.NewAuthorization(session.Subject),
		RequestMetadata: ch.events.NewRequestMetadata(ctx),
		EventStatus:     cheventspub.DeleteClusterHosts_STARTED,
		Details: &cheventspub.DeleteClusterHosts_EventDetails{
			ClusterId: cid,
			HostNames: []string{hostName},
		},
	}
	em, err := ch.events.NewEventMetadata(event, op, session.FolderCoords.CloudExtID, session.FolderCoords.FolderExtID)
	if err != nil {
		return err
	}
	event.EventMetadata = em

	if err = ch.events.Store(ctx, event, op); err != nil {
		return xerrors.Errorf("failed to save event for op %+v: %w", op, err)
	}

	return nil
}

func planAddClickHouseReplicas(
	availableZones []string,
	hostGroups map[string]*clusterslogic.HostGroup,
	targetCount optional.Int64,
) map[string][]chmodels.HostSpec {
	var sampleGroup clusterslogic.HostGroup
	for _, group := range hostGroups {
		sampleGroup = *group
		break
	}
	sampleSpecs := singleShardHosts(availableZones, &sampleGroup, targetCount)
	if len(sampleSpecs) == 0 {
		return nil
	}

	specs := map[string][]chmodels.HostSpec{}
	for shardID, group := range hostGroups {
		if group.HostsToRemove.Len() > 0 {
			continue
		}
		group.HostsToAdd = sampleGroup.HostsToAdd
		specs[shardID] = sampleSpecs
	}

	return specs
}

func singleShardHosts(
	availableZones []string,
	group *clusterslogic.HostGroup,
	targetCount optional.Int64,
) []chmodels.HostSpec {
	if !targetCount.Valid || targetCount.Int64 <= group.HostsCurrent.Len() {
		return nil
	}

	presentZones := make([]string, 0, len(group.HostsCurrent))
	for _, zone := range group.HostsCurrent {
		presentZones = append(presentZones, zone.ZoneID)
	}

	unusedZones := make([]string, 0, len(availableZones)-len(presentZones))
	for _, zone := range availableZones {
		if !slices.ContainsString(presentZones, zone) {
			unusedZones = append(unusedZones, zone)
		}
	}

	hostsToAdd := int(targetCount.Int64 - group.HostsCurrent.Len())
	zonesToAdd := unusedZones
	if len(zonesToAdd) > hostsToAdd {
		zonesToAdd = zonesToAdd[:hostsToAdd]
	}

	hostsToAdd -= len(zonesToAdd)

	for hostsToAdd >= len(availableZones) {
		zonesToAdd = append(zonesToAdd, availableZones...)
		hostsToAdd -= len(availableZones)
	}

	if hostsToAdd != 0 {
		rnd := rand.NewSource(time.Now().Unix())
		slices.Shuffle(availableZones, rnd)
		zonesToAdd = append(zonesToAdd, availableZones[:hostsToAdd]...)
	}

	specs := make([]chmodels.HostSpec, 0, len(zonesToAdd))
	for _, zone := range zonesToAdd {
		specs = append(specs, chmodels.HostSpec{
			ZoneID:         zone,
			HostRole:       group.Role,
			AssignPublicIP: true,
		})
	}

	group.HostsToAdd = chmodels.ToZoneHosts(specs)
	return specs
}

func planDeleteClickHouseReplicas(
	hostGroups map[string]*clusterslogic.HostGroup,
	shardHostMap map[string][]hosts.HostExtended,
	targetCount optional.Int64,
) ([]string, error) {
	if !targetCount.Valid {
		return nil, nil
	}

	if targetCount.Int64 <= 0 {
		return nil, semerr.InvalidInputf("invalid replica count '%d', shards must have at least one replica", targetCount.Int64)
	}

	var hostsToDelete []string

	for shard, shardHosts := range shardHostMap {
		if int64(len(shardHosts)) > targetCount.Int64 {
			sort.Slice(shardHosts, func(i, j int) bool {
				return clusterslogic.MustParseIndexFromFQDN(shardHosts[i].FQDN) < clusterslogic.MustParseIndexFromFQDN(shardHosts[j].FQDN)
			})

			shardHostsToDelete := shardHosts[targetCount.Int64:]
			hostGroups[shard].HostsToRemove = clusterslogic.ZoneHostsListFromHosts(shardHostsToDelete)
			for _, host := range shardHostsToDelete {
				hostsToDelete = append(hostsToDelete, host.FQDN)
			}
		}
	}

	return hostsToDelete, nil
}
