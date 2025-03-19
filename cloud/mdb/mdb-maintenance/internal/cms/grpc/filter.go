package grpc

import (
	"context"
	"time"

	cmsv1 "a.yandex-team.ru/cloud/mdb/cms/api/grpcapi/v1"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/sqlerrors"
	"a.yandex-team.ru/cloud/mdb/mdb-maintenance/internal/models"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (g *GRPCCMS) AwaitingPrimarySwitchover(ctx context.Context, duration time.Duration, clusterType string) ([]models.Cluster, error) {
	list, err := g.cms.List(ctx)
	if err != nil {
		return nil, err
	}
	var result []models.Cluster
	for _, operation := range list.Operations {
		if len(operation.ExecutedSteps) == 0 || operation.ExecutedSteps[len(operation.ExecutedSteps)-1] != cmsv1.InstanceOperation_CHECK_IF_PRIMARY {
			continue
		}
		if operation.CreatedAt.AsTime().Add(duration).After(time.Now()) {
			continue
		}
		cluster, err := g.meta.GetClusterByInstanceID(ctx, operation.InstanceId, clusterType)
		if err != nil {
			if xerrors.Is(err, sqlerrors.ErrNotFound) {
				continue
			}
			return nil, err
		}
		cluster.TargetMaintenanceVtypeID = operation.InstanceId
		result = append(result, cluster)
	}
	return result, err
}
