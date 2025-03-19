package provider

import (
	"context"
	"sort"
	"time"

	cheventspub "a.yandex-team.ru/cloud/bitbucket/public-api/yandex/cloud/events/mdb/clickhouse"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/clickhouse/chmodels"
	clusterslogic "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/sessions"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/pagination"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/tasks"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (ch *ClickHouse) ShardGroup(ctx context.Context, cid, name string) (chmodels.ShardGroup, error) {
	var shardGroupModel chmodels.ShardGroup
	if err := ch.operator.ReadOnCluster(ctx, cid, clusters.TypeClickHouse,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, _ clusterslogic.Cluster) error {
			sc, err := ch.chSubCluster(ctx, reader, cid)
			if err != nil {
				return err
			}

			if group, ok := sc.Pillar.Data.ClickHouse.ShardGroups[name]; ok {
				shardGroupModel.ClusterID = cid
				shardGroupModel.Name = name
				shardGroupModel.Description = group.Description
				shardGroupModel.ShardNames = group.ShardNames
				return nil
			}

			return semerr.NotFoundf("shard group %q not found", name)
		},
	); err != nil {
		return chmodels.ShardGroup{}, err
	}

	return shardGroupModel, nil
}

func (ch *ClickHouse) ShardGroups(ctx context.Context, cid string, pageSize, offset int64) ([]chmodels.ShardGroup, pagination.OffsetPageToken, error) {
	var shardGroupModels []chmodels.ShardGroup

	err := ch.operator.ReadOnCluster(ctx, cid, clusters.TypeClickHouse,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, _ clusterslogic.Cluster) error {
			sc, err := ch.chSubCluster(ctx, reader, cid)
			if err != nil {
				return err
			}

			shardGroupModels = make([]chmodels.ShardGroup, 0, len(sc.Pillar.Data.ClickHouse.ShardGroups))
			for groupName, group := range sc.Pillar.Data.ClickHouse.ShardGroups {
				shardGroupModels = append(shardGroupModels, chmodels.ShardGroup{
					ClusterID:   cid,
					Name:        groupName,
					Description: group.Description,
					ShardNames:  group.ShardNames,
				})
			}

			sort.Slice(shardGroupModels, func(i, j int) bool {
				return shardGroupModels[i].Name < shardGroupModels[j].Name
			})

			return nil
		},
	)
	if err != nil {
		return nil, pagination.OffsetPageToken{}, err
	}

	page := pagination.NewPage(int64(len(shardGroupModels)), pageSize, offset)
	nextPageToken := pagination.OffsetPageToken{
		Offset: page.NextPageOffset,
		More:   page.HasMore,
	}

	return shardGroupModels[page.LowerIndex:page.UpperIndex], nextPageToken, nil
}

func (ch *ClickHouse) CreateShardGroup(ctx context.Context, group chmodels.ShardGroup) (operations.Operation, error) {
	if err := chmodels.ValidateShardGroupName(group.Name, group.ClusterID); err != nil {
		return operations.Operation{}, err
	}

	return ch.operator.CreateOnCluster(ctx, group.ClusterID, clusters.TypeClickHouse,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, modifier clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {
			sc, err := ch.chSubCluster(ctx, reader, group.ClusterID)
			if err != nil {
				return operations.Operation{}, err
			}

			shards, err := reader.ListShards(ctx, group.ClusterID)
			if err != nil {
				return operations.Operation{}, err
			}

			group.ShardNames, err = processClusterShards(shards, group.ShardNames)
			if err != nil {
				return operations.Operation{}, err
			}

			if err = sc.Pillar.AddShardGroup(group); err != nil {
				return operations.Operation{}, err
			}

			if err = modifier.UpdateSubClusterPillar(ctx, sc.ClusterID, sc.SubClusterID, cluster.Revision, sc.Pillar); err != nil {
				return operations.Operation{}, err
			}

			op, err := ch.tasks.CreateTask(
				ctx,
				session,
				tasks.CreateTaskArgs{
					ClusterID:     group.ClusterID,
					FolderID:      session.FolderCoords.FolderID,
					Auth:          session.Subject,
					TaskType:      chmodels.TaskTypeClusterModify,
					OperationType: chmodels.OperationTypeShardGroupCreate,
					Metadata:      chmodels.MetadataCreateClusterShardGroup{ShardGroupName: group.Name},
					Revision:      cluster.Revision,
					Timeout:       optional.NewDuration(time.Hour * 3),
				},
			)
			if err != nil {
				return operations.Operation{}, xerrors.Errorf("failed to create operation: %w", err)
			}

			event := &cheventspub.CreateShardGroup{
				Authentication:  ch.events.NewAuthentication(session.Subject),
				Authorization:   ch.events.NewAuthorization(session.Subject),
				RequestMetadata: ch.events.NewRequestMetadata(ctx),
				EventStatus:     cheventspub.CreateShardGroup_STARTED,
				Details: &cheventspub.CreateShardGroup_EventDetails{
					ClusterId:      group.ClusterID,
					ShardGroupName: group.Name,
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
		},
	)
}

func (ch *ClickHouse) UpdateShardGroup(ctx context.Context, update chmodels.UpdateShardGroupArgs) (operations.Operation, error) {
	return ch.operator.ModifyOnNotStoppedCluster(ctx, update.ClusterID, clusters.TypeClickHouse,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, modifier clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {
			sc, err := ch.chSubCluster(ctx, reader, update.ClusterID)
			if err != nil {
				return operations.Operation{}, err
			}

			if update.ShardNames.Valid {
				shards, err := reader.ListShards(ctx, update.ClusterID)
				if err != nil {
					return operations.Operation{}, err
				}
				update.ShardNames.Strings, err = processClusterShards(shards, update.ShardNames.Strings)
				if err != nil {
					return operations.Operation{}, err
				}
			}

			if err := sc.Pillar.UpdateShardGroup(update); err != nil {
				return operations.Operation{}, err
			}

			if err = modifier.UpdateSubClusterPillar(ctx, sc.ClusterID, sc.SubClusterID, cluster.Revision, sc.Pillar); err != nil {
				return operations.Operation{}, err
			}

			op, err := ch.tasks.CreateTask(
				ctx,
				session,
				tasks.CreateTaskArgs{
					ClusterID:     update.ClusterID,
					FolderID:      session.FolderCoords.FolderID,
					Auth:          session.Subject,
					TaskType:      chmodels.TaskTypeClusterModify,
					OperationType: chmodels.OperationTypeShardGroupUpdate,
					Metadata:      chmodels.MetadataUpdateClusterShardGroup{ShardGroupName: update.Name},
					Revision:      cluster.Revision,
					Timeout:       optional.NewDuration(time.Hour * 3),
				},
			)
			if err != nil {
				return operations.Operation{}, xerrors.Errorf("failed to create operation: %w", err)
			}

			event := &cheventspub.UpdateShardGroup{
				Authentication:  ch.events.NewAuthentication(session.Subject),
				Authorization:   ch.events.NewAuthorization(session.Subject),
				RequestMetadata: ch.events.NewRequestMetadata(ctx),
				EventStatus:     cheventspub.UpdateShardGroup_STARTED,
				Details: &cheventspub.UpdateShardGroup_EventDetails{
					ClusterId:      update.ClusterID,
					ShardGroupName: update.Name,
				},
			}
			em, err := ch.events.NewEventMetadata(event, op, session.FolderCoords.CloudExtID, session.FolderCoords.FolderExtID)
			if err != nil {
				return operations.Operation{}, err
			}
			event.EventMetadata = em

			return op, nil
		},
	)
}

func (ch *ClickHouse) DeleteShardGroup(ctx context.Context, cid string, name string) (operations.Operation, error) {
	return ch.operator.DeleteOnCluster(ctx, cid, clusters.TypeClickHouse,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, modifier clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {
			sc, err := ch.chSubCluster(ctx, reader, cid)
			if err != nil {
				return operations.Operation{}, err
			}

			if err = sc.Pillar.DeleteShardGroup(name); err != nil {
				return operations.Operation{}, err
			}

			if err = modifier.UpdateSubClusterPillar(ctx, sc.ClusterID, sc.SubClusterID, cluster.Revision, sc.Pillar); err != nil {
				return operations.Operation{}, err
			}

			op, err := ch.tasks.CreateTask(
				ctx,
				session,
				tasks.CreateTaskArgs{
					ClusterID:     cid,
					FolderID:      session.FolderCoords.FolderID,
					Auth:          session.Subject,
					TaskType:      chmodels.TaskTypeClusterModify,
					OperationType: chmodels.OperationTypeShardGroupDelete,
					Metadata:      chmodels.MetadataDeleteClusterShardGroup{ShardGroupName: name},
					Revision:      cluster.Revision,
				},
			)
			if err != nil {
				return operations.Operation{}, xerrors.Errorf("failed to create operation: %w", err)
			}

			event := &cheventspub.DeleteShardGroup{
				Authentication:  ch.events.NewAuthentication(session.Subject),
				Authorization:   ch.events.NewAuthorization(session.Subject),
				RequestMetadata: ch.events.NewRequestMetadata(ctx),
				EventStatus:     cheventspub.DeleteShardGroup_STARTED,
				Details: &cheventspub.DeleteShardGroup_EventDetails{
					ClusterId:      cid,
					ShardGroupName: name,
				},
			}
			em, err := ch.events.NewEventMetadata(event, op, session.FolderCoords.CloudExtID, session.FolderCoords.FolderExtID)
			if err != nil {
				return operations.Operation{}, err
			}
			event.EventMetadata = em

			return op, nil
		},
	)
}

func processClusterShards(shards []clusters.Shard, shardNames []string) ([]string, error) {
	if len(shardNames) == 0 {
		return nil, semerr.InvalidInput("shard list can not be empty")
	}

	var result []string
	uniq := map[string]struct{}{}

	for _, name := range shardNames {
		if _, ok := uniq[name]; !ok {
			uniq[name] = struct{}{}
			result = append(result, name)
			found := false
			for _, shard := range shards {
				if name == shard.Name {
					found = true
					break
				}
			}

			if !found {
				return nil, semerr.NotFoundf("shard %q not found", name)
			}
		}
	}

	sort.Strings(result)
	return result, nil
}
