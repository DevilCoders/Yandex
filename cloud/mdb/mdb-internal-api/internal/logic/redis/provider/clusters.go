package provider

import (
	"context"
	"time"

	reventspub "a.yandex-team.ru/cloud/bitbucket/public-api/yandex/cloud/events/mdb/redis"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	clusterslogic "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/sessions"
	taskslogic "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/tasks"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/redis/rmodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (r *Redis) StartCluster(ctx context.Context, cid string) (operations.Operation, error) {
	return r.operator.ModifyOnNotRunningCluster(ctx, cid, clusters.TypeRedis,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, modifier clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {
			taskArgs := map[string]interface{}{
				"time_limit": optional.NewDuration(3 * time.Hour),
			}

			op, err := r.tasks.StartCluster(
				ctx,
				session,
				cid,
				cluster.Revision,
				rmodels.TaskTypeClusterStart,
				rmodels.OperationTypeClusterStart,
				rmodels.MetadataStartCluster{},
				taskslogic.StartClusterTaskArgs(taskArgs),
			)
			if err != nil {
				return operations.Operation{}, err
			}

			event := &reventspub.StartCluster{
				Authentication:  r.events.NewAuthentication(session.Subject),
				Authorization:   r.events.NewAuthorization(session.Subject),
				RequestMetadata: r.events.NewRequestMetadata(ctx),
				EventStatus:     reventspub.StartCluster_STARTED,
				Details: &reventspub.StartCluster_EventDetails{
					ClusterId: cid,
				},
			}
			em, err := r.events.NewEventMetadata(event, op, session.FolderCoords.CloudExtID, session.FolderCoords.FolderExtID)
			if err != nil {
				return operations.Operation{}, err
			}
			event.EventMetadata = em

			if err = r.events.Store(ctx, event, op); err != nil {
				return operations.Operation{}, xerrors.Errorf("failed to save event for op %+v: %w", op, err)
			}

			return op, nil
		})
}

func (r *Redis) StopCluster(ctx context.Context, cid string) (operations.Operation, error) {
	return r.operator.ModifyOnNotStoppedCluster(ctx, cid, clusters.TypeRedis,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, modifier clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {
			op, err := r.tasks.StopCluster(
				ctx,
				session,
				cid,
				cluster.Revision,
				rmodels.TaskTypeClusterStop,
				rmodels.OperationTypeClusterStop,
				rmodels.MetadataStopCluster{},
			)
			if err != nil {
				return operations.Operation{}, err
			}

			event := &reventspub.StopCluster{
				Authentication:  r.events.NewAuthentication(session.Subject),
				Authorization:   r.events.NewAuthorization(session.Subject),
				RequestMetadata: r.events.NewRequestMetadata(ctx),
				EventStatus:     reventspub.StopCluster_STARTED,
				Details: &reventspub.StopCluster_EventDetails{
					ClusterId: cid,
				},
			}
			em, err := r.events.NewEventMetadata(event, op, session.FolderCoords.CloudExtID, session.FolderCoords.FolderExtID)
			if err != nil {
				return operations.Operation{}, err
			}
			event.EventMetadata = em

			if err = r.events.Store(ctx, event, op); err != nil {
				return operations.Operation{}, xerrors.Errorf("failed to save event for op %+v: %w", op, err)
			}

			return op, nil
		})
}

func (r *Redis) MoveCluster(ctx context.Context, cid, destinationFolderID string) (operations.Operation, error) {
	return r.operator.MoveCluster(ctx, cid, destinationFolderID, clusters.TypeRedis,
		func(ctx context.Context, session sessions.Session, modifier clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {
			err := modifier.UpdateClusterFolder(ctx, cluster, destinationFolderID)
			if err != nil {
				return operations.Operation{}, err
			}

			op, err := r.tasks.MoveCluster(
				ctx,
				session,
				cid,
				cluster.Revision,
				rmodels.OperationTypeClusterMove,
				rmodels.MetadataMoveCluster{
					SourceFolderID:      session.FolderCoords.FolderExtID,
					DestinationFolderID: destinationFolderID,
				},
				func(options *taskslogic.MoveClusterOptions) {
					options.TaskArgs = map[string]interface{}{}
					options.SrcFolderTaskType = rmodels.TaskTypeClusterMove
					options.DstFolderTaskType = rmodels.TaskTypeClusterMoveNoOp
					options.SrcFolderCoords = session.FolderCoords
					options.DstFolderExtID = destinationFolderID
				},
			)
			if err != nil {
				return operations.Operation{}, err
			}

			event := &reventspub.MoveCluster{
				Authentication:  r.events.NewAuthentication(session.Subject),
				Authorization:   r.events.NewAuthorization(session.Subject),
				RequestMetadata: r.events.NewRequestMetadata(ctx),
				EventStatus:     reventspub.MoveCluster_STARTED,
				Details: &reventspub.MoveCluster_EventDetails{
					ClusterId: cid,
				},
			}
			em, err := r.events.NewEventMetadata(event, op, session.FolderCoords.CloudExtID, session.FolderCoords.FolderExtID)
			if err != nil {
				return operations.Operation{}, err
			}
			event.EventMetadata = em

			if err = r.events.Store(ctx, event, op); err != nil {
				return operations.Operation{}, xerrors.Errorf("failed to save event for op %+v: %w", op, err)
			}

			return op, nil
		})
}

func (r *Redis) BackupCluster(ctx context.Context, cid string) (operations.Operation, error) {
	return r.operator.ModifyOnNotStoppedCluster(ctx, cid, clusters.TypeRedis,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, modifier clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {

			op, err := r.tasks.BackupCluster(
				ctx,
				session,
				cid,
				cluster.Revision,
				rmodels.TaskTypeClusterBackup,
				rmodels.OperationTypeClusterBackup,
				rmodels.MetadataBackupCluster{},
			)
			if err != nil {
				return operations.Operation{}, err
			}

			event := &reventspub.BackupCluster{
				Authentication:  r.events.NewAuthentication(session.Subject),
				Authorization:   r.events.NewAuthorization(session.Subject),
				RequestMetadata: r.events.NewRequestMetadata(ctx),
				EventStatus:     reventspub.BackupCluster_STARTED,
				Details: &reventspub.BackupCluster_EventDetails{
					ClusterId: cid,
				},
			}
			em, err := r.events.NewEventMetadata(event, op, session.FolderCoords.CloudExtID, session.FolderCoords.FolderExtID)
			if err != nil {
				return operations.Operation{}, err
			}
			event.EventMetadata = em

			if err = r.events.Store(ctx, event, op); err != nil {
				return operations.Operation{}, xerrors.Errorf("failed to save event for op %+v: %w", op, err)
			}

			return op, nil
		},
	)
}
