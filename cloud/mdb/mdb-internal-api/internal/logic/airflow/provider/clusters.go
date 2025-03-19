package provider

import (
	"context"
	"time"

	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/airflow/afmodels"
	clusterslogic "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/sessions"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
)

const (
	airflowService = "managed-airflow"
)

func (af *Airflow) MDBCluster(ctx context.Context, cid string) (afmodels.MDBCluster, error) {
	var cluster afmodels.MDBCluster
	if err := af.operator.ReadCluster(ctx, cid,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader) error {
			cl, err := reader.ClusterExtendedByClusterID(ctx, cid, clusters.TypeAirflow, models.VisibilityVisible, session)
			if err != nil {
				return err
			}

			pillar, err := getPillarFromJSON(cl.Pillar)
			if err != nil {
				return err
			}

			var triggerer *afmodels.TriggererConfig
			if pillar.Values.Triggerer != nil {
				triggerer = &afmodels.TriggererConfig{
					Count:     pillar.Values.Triggerer.Replicas,
					Resources: pillar.Values.Triggerer.Resources.ToModel(),
				}
			}

			cluster = afmodels.MDBCluster{
				ClusterExtended: cl,
				Config: afmodels.ClusterConfigSpec{
					Version: pillar.Values.Version,
					Airflow: afmodels.AirflowConfig{
						Config: pillar.Values.Config,
					},
					Webserver: afmodels.WebserverConfig{
						Count:     pillar.Values.Webserver.Replicas,
						Resources: pillar.Values.Webserver.Resources.ToModel(),
					},
					Scheduler: afmodels.SchedulerConfig{
						Count:     pillar.Values.Scheduler.Replicas,
						Resources: pillar.Values.Scheduler.Resources.ToModel(),
					},
					Triggerer: triggerer,
					Worker: afmodels.WorkerConfig{
						MinCount:        pillar.Values.Worker.KEDA.MinReplicaCount,
						MaxCount:        pillar.Values.Worker.KEDA.MaxReplicaCount,
						CooldownPeriod:  time.Duration(pillar.Values.Worker.KEDA.CooldownPeriodSec) * time.Second,
						PollingInterval: time.Duration(pillar.Values.Worker.KEDA.PollingIntervalSec) * time.Second,
						Resources:       pillar.Values.Worker.Resources.ToModel(),
					},
				},
				SubnetIDs: pillar.Data.Kubernetes.SubnetIDs,
				CodeSync: afmodels.CodeSyncConfig{
					Bucket: pillar.Data.S3Bucket,
				},
			}
			return nil
		},
	); err != nil {
		return afmodels.MDBCluster{}, err
	}

	return cluster, nil
}

func searchAttributesExtractor(cluster clusterslogic.Cluster) (map[string]interface{}, error) {
	pillar, err := getPillarFromCluster(cluster)
	if err != nil {
		return nil, err
	}
	searchAttributes := make(map[string]interface{})

	// TODO: Better search attributes
	for k, v := range pillar.Values.Config {
		searchAttributes[k] = v
	}

	return searchAttributes, nil
}
