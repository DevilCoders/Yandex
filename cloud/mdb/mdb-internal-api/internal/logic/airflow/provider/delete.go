package provider

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/airflow/afmodels"
	clusterslogic "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/sessions"
	taskslogic "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/tasks"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
)

func (af *Airflow) DeleteCluster(ctx context.Context, cid string) (operations.Operation, error) {
	nodeGroupIDs := make([]string, 0)

	_, err := af.operator.ModifyOnCluster(ctx, cid, clusters.TypeAirflow,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, modifier clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {
			subClusters, err := reader.KubernetesSubClusters(ctx, cid)
			if err != nil {
				return operations.Operation{}, err
			}

			for _, subCluster := range subClusters {
				if subCluster.NodeGroupID != "" {
					nodeGroupIDs = append(nodeGroupIDs, subCluster.NodeGroupID)
				}
				err := modifier.DeleteSubCluster(ctx, cid, subCluster.SubCluster.SubClusterID, cluster.Revision)
				if err != nil {
					return operations.Operation{}, err
				}
			}
			return operations.Operation{}, nil
		},
	)
	if err != nil {
		return operations.Operation{}, err
	}

	return af.operator.Delete(ctx, cid, clusters.TypeAirflow,
		func(ctx context.Context, session sessions.Session, cluster clusterslogic.Cluster, _ clusterslogic.Reader) (operations.Operation, error) {

			opts := taskslogic.DeleteTaskArgs(map[string]interface{}{
				"node_group_ids": nodeGroupIDs,
			})

			op, err := af.tasks.DeleteCluster(
				ctx,
				session,
				cluster.ClusterID,
				cluster.Revision,
				taskslogic.DeleteClusterTaskTypes{
					Delete:   afmodels.TaskTypeClusterDelete,
					Metadata: afmodels.TaskTypeClusterDeleteMetadata,
				},
				afmodels.OperationTypeClusterDelete,
				taskslogic.DeleteClusterS3Buckets{},
				opts,
			)
			if err != nil {
				return operations.Operation{}, err
			}
			return op, nil
		},
	)
}
