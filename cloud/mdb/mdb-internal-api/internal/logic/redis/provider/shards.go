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
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/tasks"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (r *Redis) Rebalance(ctx context.Context, cid string) (operations.Operation, error) {
	return r.operator.ModifyOnCluster(ctx, cid, clusters.TypeRedis,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, _ clusterslogic.Modifier,
			cl clusterslogic.Cluster) (operations.Operation, error) {
			var pillar rpillars.Cluster
			err := cl.Pillar(&pillar)
			if err != nil {
				return operations.Operation{}, err
			}

			if !pillar.IsSharded() {
				return operations.Operation{}, semerr.FailedPrecondition(
					"sharding must be enabled")
			}

			op, err := r.tasks.CreateTask(
				ctx,
				session,
				tasks.CreateTaskArgs{
					ClusterID:     cid,
					FolderID:      session.FolderCoords.FolderID,
					Auth:          session.Subject,
					TaskType:      rmodels.TaskTypeRebalance,
					OperationType: rmodels.OperationTypeRebalance,
					Metadata:      rmodels.MetadataRebalance{},

					Timeout:  optional.NewDuration(time.Hour * 24),
					Revision: cl.Revision,
				},
			)
			if err != nil {
				return operations.Operation{}, err
			}

			err = r.sendRebalanceEvent(ctx, session, cl.ClusterID, op)
			if err != nil {
				return operations.Operation{}, err
			}

			return op, nil
		},
	)
}

func (r *Redis) sendRebalanceEvent(ctx context.Context, session sessions.Session, cid string,
	op operations.Operation) error {
	event := &reventspub.RebalanceCluster{
		Authentication:  r.events.NewAuthentication(session.Subject),
		Authorization:   r.events.NewAuthorization(session.Subject),
		RequestMetadata: r.events.NewRequestMetadata(ctx),
		EventStatus:     reventspub.RebalanceCluster_STARTED,
		Details: &reventspub.RebalanceCluster_EventDetails{
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
