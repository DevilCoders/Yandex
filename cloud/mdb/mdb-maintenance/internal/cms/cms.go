package cms

import (
	"context"
	"time"

	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/instanceclient"
	"a.yandex-team.ru/cloud/mdb/mdb-maintenance/internal/models"
)

type CMS interface {
	AwaitingPrimarySwitchover(ctx context.Context, duration time.Duration, clusterType string) ([]models.Cluster, error)
}

func SelectFromCms(ctx context.Context, cmsapi CMS, selections []models.CMSClusterSelection) ([]models.Cluster, error) {
	var result []models.Cluster
	for _, selection := range selections {
		if selection.Steps.EndsWith == instanceclient.StepNameCheckIfPrimary {
			clusters, err := cmsapi.AwaitingPrimarySwitchover(ctx, selection.Duration, selection.ClusterType)
			if err != nil {
				return nil, err
			}
			result = append(result, clusters...)
		}
	}
	return result, nil
}
