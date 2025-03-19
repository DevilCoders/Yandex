package provider

import (
	"context"
	"time"

	reventspub "a.yandex-team.ru/cloud/bitbucket/public-api/yandex/cloud/events/mdb/redis"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	clusterslogic "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/sessions"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/redis/provider/internal/rpillars"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/redis/rmodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/pagination"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/tasks"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (r *Redis) ListHosts(ctx context.Context, cid string, pageSize, offset int64) ([]hosts.HostExtended, pagination.OffsetPageToken, error) {
	var (
		hostList  []hosts.HostExtended
		newOffset int64
		more      bool
	)
	if err := r.operator.ReadOnCluster(ctx, cid, clusters.TypeRedis,
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
				var hostPillar rpillars.Host
				err = reader.HostPillar(ctx, host.FQDN, &hostPillar)
				if err != nil {
					return err
				}

				hostList[i].ReplicaPriority = hostPillar.Data.Redis.Config.ReplicaPriority
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

func (r *Redis) StartFailover(ctx context.Context, cid string, hostNames []string) (operations.Operation, error) {
	// Check input first
	if len(hostNames) > 1 {
		return operations.Operation{},
			semerr.FailedPrecondition("Failover on redis cluster does not support multiple hostnames at this time.")
	}

	return r.operator.ModifyOnCluster(ctx, cid, clusters.TypeRedis,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, _ clusterslogic.Modifier,
			cl clusterslogic.Cluster) (operations.Operation, error) {
			rCluster, err := r.rCluster(cl)
			if err != nil {
				return operations.Operation{}, err
			}

			currentHosts, err := clusterslogic.ListAllHosts(ctx, reader, cl.ClusterID)
			if err != nil {
				return operations.Operation{}, err
			}

			if len(currentHosts) == 1 {
				return operations.Operation{},
					semerr.FailedPrecondition("Operation is not permitted on a single node cluster.")
			}

			if rCluster.Pillar.IsSharded() && len(hostNames) < 1 {
				return operations.Operation{}, semerr.FailedPrecondition(
					"To perform failover on sharded redis cluster at least one host must be specified.")
			}

			if !rCluster.Pillar.IsSharded() && len(hostNames) < 1 {
				masters := GetMasters(currentHosts)
				if len(masters) == 0 {
					return operations.Operation{}, semerr.FailedPrecondition(
						"Unable to find masters for cluster. Failover is not safe. Aborting failover.")
				}
				hostNames = masters
			}

			if isNotSubset(hostNames, currentHosts) {
				return operations.Operation{},
					semerr.FailedPrecondition("Hostnames you specified are not the part of cluster.")
			}

			op, err := r.tasks.CreateTask(
				ctx,
				session,
				tasks.CreateTaskArgs{
					ClusterID:     cid,
					FolderID:      session.FolderCoords.FolderID,
					Auth:          session.Subject,
					TaskType:      rmodels.TaskTypeFailover,
					OperationType: rmodels.OperationTypeStartFailover,
					Metadata: rmodels.MetadataFailoverHosts{
						HostNames: hostNames,
					},
					TaskArgs: map[string]interface{}{
						"failover_hosts": hostNames,
					},

					Timeout:  optional.NewDuration(time.Hour),
					Revision: cl.Revision,
				},
			)
			if err != nil {
				return operations.Operation{}, err
			}

			err = r.sendStartClusterFailoverEvent(ctx, session, cl.ClusterID, op)
			if err != nil {
				return operations.Operation{}, err
			}

			return op, nil
		},
	)
}

func isNotSubset(hostNames []string, currentHosts []hosts.HostExtended) bool {
	currentMap := make(map[string]struct{})
	for _, currentHost := range currentHosts {
		currentMap[currentHost.FQDN] = struct{}{}
	}
	for _, hostName := range hostNames {
		if _, contains := currentMap[hostName]; contains {
			return false
		}
	}
	return true
}

func (r *Redis) sendStartClusterFailoverEvent(ctx context.Context, session sessions.Session, cid string,
	op operations.Operation) error {
	event := &reventspub.StartClusterFailover{
		Authentication:  r.events.NewAuthentication(session.Subject),
		Authorization:   r.events.NewAuthorization(session.Subject),
		RequestMetadata: r.events.NewRequestMetadata(ctx),
		EventStatus:     reventspub.StartClusterFailover_STARTED,
		Details: &reventspub.StartClusterFailover_EventDetails{
			ClusterId: cid,
		},
	}
	em, err := r.events.NewEventMetadata(event, op, session.FolderCoords.CloudExtID, session.FolderCoords.FolderExtID)
	if err != nil {
		return err
	}
	event.EventMetadata = em

	if err = r.events.Store(ctx, event, op); err != nil {
		return xerrors.Errorf("failed to save event for op %+v: %w", op, err)
	}
	return nil
}

func (r *Redis) AddHosts(ctx context.Context, cid string, specs []rmodels.HostSpec) (operations.Operation, error) {
	if len(specs) > 1 {
		return operations.Operation{}, semerr.NotImplemented("adding multiple hosts at once is not supported yet")
	}

	if len(specs) < 1 {
		return operations.Operation{}, semerr.InvalidInput("no hosts to add are specified")
	}

	spec := specs[0]
	return r.operator.ModifyOnNotStoppedCluster(ctx, cid, clusters.TypeRedis,
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

			shardHosts := make([]hosts.HostExtended, 0, len(currentHosts))
			for _, h := range currentHosts {
				if h.ShardID.Must() == targetShard.ShardID {
					shardHosts = append(shardHosts, h)
				}
			}

			if len(shardHosts) == 0 {
				return operations.Operation{}, semerr.FailedPrecondition("there no hosts in shard")
			}

			rCluster, err := r.rCluster(cluster)
			if err != nil {
				return operations.Operation{}, err
			}

			shardNum, hostGroups := buildHostGroups(currentHosts, targetShard.ShardID,
				clusterslogic.ZoneHostsList{clusterslogic.ZoneHosts{ZoneID: spec.ZoneID, Count: 1}}, nil)
			resolvedHostGroups, err := validateClusterNodes(ctx, session, modifier, shardNum, *rCluster.Pillar, hostGroups...)
			if err != nil {
				return operations.Operation{}, err
			}

			_, subnets, err := r.compute.NetworkAndSubnets(ctx, cluster.NetworkID)
			if err != nil {
				return operations.Operation{}, err
			}

			createdHosts, err := r.createHosts(
				ctx,
				session,
				reader,
				modifier,
				rCluster,
				targetShard.SubClusterID,
				optional.NewString(targetShard.ShardID),
				targetShard.Name,
				[]rmodels.HostSpec{spec},
				subnets,
				resolvedHostGroups.SingleWithChanges(),
			)
			if err != nil {
				return operations.Operation{}, err
			}
			if len(createdHosts) != 1 {
				return operations.Operation{}, semerr.Internalf("%d hosts created instead of 1", len(createdHosts))
			}
			fqdn := createdHosts[0].FQDN

			taskType := rmodels.TaskTypeHostCreate
			if shardNum > 1 {
				taskType = rmodels.TaskTypeShardHostCreate
			}

			timeout := time.Hour * 24
			op, err := r.tasks.CreateTask(
				ctx,
				session,
				tasks.CreateTaskArgs{
					ClusterID:     cid,
					FolderID:      session.FolderCoords.FolderID,
					Auth:          session.Subject,
					TaskType:      taskType,
					OperationType: rmodels.OperationTypeHostCreate,
					Revision:      cluster.Revision,
					Timeout:       optional.NewDuration(timeout),
					Metadata: rmodels.MetadataHostCreate{
						HostNames: []string{fqdn},
					},
					TaskArgs: map[string]interface{}{
						"host":     fqdn,
						"subcid":   createdHosts[0].SubClusterID,
						"shard_id": createdHosts[0].ShardID.Must(),
					},
				})
			if err != nil {
				return operations.Operation{}, xerrors.Errorf("failed to create operation: %w", err)
			}

			if err := r.createAddHostsEvent(ctx, session, op, cid, fqdn); err != nil {
				return operations.Operation{}, err
			}

			return op, nil
		})
}

func (r *Redis) createAddHostsEvent(ctx context.Context, session sessions.Session, op operations.Operation, cid, hostName string) error {
	event := &reventspub.AddClusterHosts{
		Authentication:  r.events.NewAuthentication(session.Subject),
		Authorization:   r.events.NewAuthorization(session.Subject),
		RequestMetadata: r.events.NewRequestMetadata(ctx),
		EventStatus:     reventspub.AddClusterHosts_STARTED,
		Details: &reventspub.AddClusterHosts_EventDetails{
			ClusterId: cid,
			HostNames: []string{hostName},
		},
	}
	em, err := r.events.NewEventMetadata(event, op, session.FolderCoords.CloudExtID, session.FolderCoords.FolderExtID)
	if err != nil {
		return err
	}
	event.EventMetadata = em

	if err = r.events.Store(ctx, event, op); err != nil {
		return xerrors.Errorf("failed to save event for op %+v: %w", op, err)
	}

	return nil
}
