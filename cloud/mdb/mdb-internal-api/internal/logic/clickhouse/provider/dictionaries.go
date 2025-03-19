package provider

import (
	"context"

	cheventspub "a.yandex-team.ru/cloud/bitbucket/public-api/yandex/cloud/events/mdb/clickhouse"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/clickhouse/chmodels"
	clusterslogic "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/sessions"
	clustermodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/tasks"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (ch *ClickHouse) CreateExternalDictionary(ctx context.Context, cid string, dict chmodels.Dictionary) (operations.Operation, error) {
	return ch.operator.ModifyOnCluster(ctx, cid, clustermodels.TypeClickHouse, func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, modifier clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {
		subcluster, err := ch.chSubCluster(ctx, reader, cid)
		if err != nil {
			return operations.Operation{}, err
		}

		if err := subcluster.Pillar.Data.ClickHouse.Config.AddDictionary(ch.cryptoProvider, dict); err != nil {
			return operations.Operation{}, err
		}

		if err := modifier.UpdateSubClusterPillar(ctx, cid, subcluster.SubClusterID, cluster.Revision, subcluster.Pillar); err != nil {
			return operations.Operation{}, err
		}

		shards, err := reader.ListShards(ctx, cid)
		if err != nil {
			return operations.Operation{}, err
		}

		for _, shard := range shards {
			shard, err := ch.chShard(ctx, reader, shard.ShardID)
			if err != nil {
				return operations.Operation{}, err
			}

			if err := shard.Pillar.Data.ClickHouse.Config.AddDictionary(ch.cryptoProvider, dict); err != nil {
				return operations.Operation{}, err
			}

			if err := modifier.UpdateShardPillar(ctx, cluster.ClusterID, shard.ShardID, cluster.Revision, shard.Pillar); err != nil {
				return operations.Operation{}, err
			}
		}

		op, err := ch.tasks.CreateTask(
			ctx,
			session,
			tasks.CreateTaskArgs{
				ClusterID:     cid,
				FolderID:      session.FolderCoords.FolderID,
				Auth:          session.Subject,
				TaskType:      chmodels.TaskTypeDictionaryCreate,
				OperationType: chmodels.OperationTypeDictionaryCreate,
				Metadata:      chmodels.MetadataCreateDictionary{},
				Revision:      cluster.Revision,
				TaskArgs: map[string]interface{}{
					"target-dictionary": dict.Name,
				},
			},
		)
		if err != nil {
			return operations.Operation{}, xerrors.Errorf("failed to create operation: %w", err)
		}

		event := &cheventspub.CreateClusterExternalDictionary{
			Authentication:  ch.events.NewAuthentication(session.Subject),
			Authorization:   ch.events.NewAuthorization(session.Subject),
			RequestMetadata: ch.events.NewRequestMetadata(ctx),
			EventStatus:     cheventspub.CreateClusterExternalDictionary_STARTED,
			Details: &cheventspub.CreateClusterExternalDictionary_EventDetails{
				ClusterId: cid,
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

func (ch *ClickHouse) UpdateExternalDictionary(ctx context.Context, cid string, dict chmodels.Dictionary) (operations.Operation, error) {
	return ch.operator.ModifyOnCluster(ctx, cid, clustermodels.TypeClickHouse, func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, modifier clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {
		subcluster, err := ch.chSubCluster(ctx, reader, cid)
		if err != nil {
			return operations.Operation{}, err
		}

		if err := subcluster.Pillar.Data.ClickHouse.Config.UpdateDictionary(ch.cryptoProvider, dict); err != nil {
			return operations.Operation{}, err
		}

		if err := modifier.UpdateSubClusterPillar(ctx, cid, subcluster.SubClusterID, cluster.Revision, subcluster.Pillar); err != nil {
			return operations.Operation{}, err
		}

		shards, err := reader.ListShards(ctx, cid)
		if err != nil {
			return operations.Operation{}, err
		}

		for _, shard := range shards {
			shard, err := ch.chShard(ctx, reader, shard.ShardID)
			if err != nil {
				return operations.Operation{}, err
			}

			if err := shard.Pillar.Data.ClickHouse.Config.UpdateDictionary(ch.cryptoProvider, dict); err != nil {
				return operations.Operation{}, err
			}

			if err := modifier.UpdateShardPillar(ctx, cluster.ClusterID, shard.ShardID, cluster.Revision, shard.Pillar); err != nil {
				return operations.Operation{}, err
			}
		}

		op, err := ch.tasks.CreateTask(
			ctx,
			session,
			tasks.CreateTaskArgs{
				ClusterID:     cid,
				FolderID:      session.FolderCoords.FolderID,
				Auth:          session.Subject,
				TaskType:      chmodels.TaskTypeDictionaryCreate,
				OperationType: chmodels.OperationTypeDictionaryUpdate,
				Metadata:      chmodels.MetadataUpdateDictionary{},
				Revision:      cluster.Revision,
				TaskArgs: map[string]interface{}{
					"target-dictionary": dict.Name,
				},
			},
		)
		if err != nil {
			return operations.Operation{}, xerrors.Errorf("failed to create operation: %w", err)
		}

		event := &cheventspub.UpdateClusterExternalDictionary{
			Authentication:  ch.events.NewAuthentication(session.Subject),
			Authorization:   ch.events.NewAuthorization(session.Subject),
			RequestMetadata: ch.events.NewRequestMetadata(ctx),
			EventStatus:     cheventspub.UpdateClusterExternalDictionary_STARTED,
			Details: &cheventspub.UpdateClusterExternalDictionary_EventDetails{
				ClusterId: cid,
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

func (ch *ClickHouse) DeleteExternalDictionary(ctx context.Context, cid string, name string) (operations.Operation, error) {
	return ch.operator.ModifyOnCluster(ctx, cid, clustermodels.TypeClickHouse, func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, modifier clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {
		subcluster, err := ch.chSubCluster(ctx, reader, cid)
		if err != nil {
			return operations.Operation{}, err
		}

		if err := subcluster.Pillar.Data.ClickHouse.Config.DeleteDictionary(name); err != nil {
			return operations.Operation{}, err
		}

		if err := modifier.UpdateSubClusterPillar(ctx, cid, subcluster.SubClusterID, cluster.Revision, subcluster.Pillar); err != nil {
			return operations.Operation{}, err
		}

		shards, err := reader.ListShards(ctx, cid)
		if err != nil {
			return operations.Operation{}, err
		}

		for _, shard := range shards {
			shard, err := ch.chShard(ctx, reader, shard.ShardID)
			if err != nil {
				return operations.Operation{}, err
			}

			if err := shard.Pillar.Data.ClickHouse.Config.DeleteDictionary(name); err != nil {
				return operations.Operation{}, err
			}

			if err := modifier.UpdateShardPillar(ctx, cluster.ClusterID, shard.ShardID, cluster.Revision, shard.Pillar); err != nil {
				return operations.Operation{}, err
			}
		}

		op, err := ch.tasks.CreateTask(
			ctx,
			session,
			tasks.CreateTaskArgs{
				ClusterID:     cid,
				FolderID:      session.FolderCoords.FolderID,
				Auth:          session.Subject,
				TaskType:      chmodels.TaskTypeDictionaryDelete,
				OperationType: chmodels.OperationTypeDictionaryDelete,
				Metadata:      chmodels.MetadataDeleteDictionary{},
				Revision:      cluster.Revision,
				TaskArgs: map[string]interface{}{
					"target-dictionary": name,
				},
			},
		)
		if err != nil {
			return operations.Operation{}, xerrors.Errorf("failed to create operation: %w", err)
		}

		event := &cheventspub.DeleteClusterExternalDictionary{
			Authentication:  ch.events.NewAuthentication(session.Subject),
			Authorization:   ch.events.NewAuthorization(session.Subject),
			RequestMetadata: ch.events.NewRequestMetadata(ctx),
			EventStatus:     cheventspub.DeleteClusterExternalDictionary_STARTED,
			Details: &cheventspub.DeleteClusterExternalDictionary_EventDetails{
				ClusterId: cid,
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
