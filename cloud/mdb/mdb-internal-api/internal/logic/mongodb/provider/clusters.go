package provider

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	clusterslogic "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/sessions"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/mongodb"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/mongodb/mongomodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
)

const (
	mongoDBService = "managed-mongodb"
)

func (mg *MongoDB) Cluster(ctx context.Context, cid string) (clusters.ClusterExtended, error) {
	return clusters.ClusterExtended{}, semerr.NotImplemented("not implemented")
}

func (mg *MongoDB) Clusters(ctx context.Context, folderExtID string, limit, offset int64) ([]clusters.ClusterExtended, error) {
	return nil, semerr.NotImplemented("not implemented")
}

func (mg *MongoDB) CreateCluster(ctx context.Context, args mongodb.CreateClusterArgs) (operations.Operation, error) {
	return mg.operator.Create(ctx, args.FolderExtID,
		func(ctx context.Context, session sessions.Session, creator clusterslogic.Creator) (clusters.Cluster, operations.Operation, error) {

			// TODO: add validation

			// TODO: map environment name

			cluster, _, err := creator.CreateCluster(ctx, models.CreateClusterArgs{
				Name:        args.Name,
				ClusterType: clusters.TypeMongoDB,
				Environment: args.Environment,
				// TODO: set PublicKey
				NetworkID: args.NetworkID,
				// TODO: set FolderID
				// TODO: set Pillar
				Description:        args.Description,
				Labels:             args.Labels,
				DeletionProtection: args.DeletionProtection,
			})
			if err != nil {
				return clusters.Cluster{}, operations.Operation{}, err
			}

			// TODO: create subclusters, initial shard and hosts

			op, err := mg.tasks.CreateCluster(
				ctx,
				session,
				cluster.ClusterID,
				cluster.Revision,
				mongomodels.TaskTypeClusterCreate,
				mongomodels.OperationTypeClusterCreate,
				mongomodels.MetadataCreateCluster{},
				optional.String{},
				nil, // securityGroupIDs (don't add them, cause don't validate them)
				mongoDBService,
				func(cluster clusterslogic.Cluster) (map[string]interface{}, error) { return nil, nil }, // TODO: extract attributes
			)
			if err != nil {
				return clusters.Cluster{}, operations.Operation{}, err
			}

			return cluster, op, nil
		},
	)
}

func (mg *MongoDB) DeleteCluster(ctx context.Context, cid string) (operations.Operation, error) {
	return operations.Operation{}, semerr.NotImplemented("not implemented")
}
