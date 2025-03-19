package provider

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	clusterslogic "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/sessions"
	taskslogic "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/tasks"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/metastore/models"
	common_models "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
	"a.yandex-team.ru/library/go/core/xerrors"
)

const (
	metastoreService = "metastore"
)

func (ms *Metastore) MDBCluster(ctx context.Context, cid string) (models.MDBCluster, error) {
	var cluster models.MDBCluster
	if err := ms.operator.ReadCluster(ctx, cid,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader) error {
			cl, err := reader.ClusterExtendedByClusterID(ctx, cid, clusters.TypeMetastore, common_models.VisibilityVisible, session)
			if err != nil {
				return err
			}

			pillar, err := getPillarFromJSON(cl.Pillar)
			if err != nil {
				return err
			}

			cluster = models.MDBCluster{
				ClusterExtended: cl,
				Config: models.MDBClusterSpec{
					SubnetIDs:         pillar.Data.UserSubnetIDs,
					Version:           pillar.Data.Version,
					MinServersPerZone: pillar.Data.MinServersPerZone,
					MaxServersPerZone: pillar.Data.MaxServersPerZone,
					NetworkID:         pillar.Data.NetworkID,
					EndpointIP:        pillar.Data.EndpointIP,
				},
			}
			return nil
		},
	); err != nil {
		return models.MDBCluster{}, err
	}

	return cluster, nil
}

func (ms *Metastore) MDBClusters(ctx context.Context, folderExtID string, limit, offset int64) ([]models.MDBCluster, error) {
	var res []models.MDBCluster
	if err := ms.operator.ReadOnFolder(ctx, folderExtID,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader) error {
			if limit <= 0 {
				limit = 100
			}

			cls, err := reader.ClustersExtended(
				ctx,
				common_models.ListClusterArgs{
					ClusterType: clusters.TypeMetastore,
					FolderID:    session.FolderCoords.FolderID,
					Limit:       optional.NewInt64(limit),
					Offset:      offset,
					Visibility:  common_models.VisibilityVisible,
				},
				session,
			)
			if err != nil {
				return xerrors.Errorf("failed to retrieve clusters: %w", err)
			}

			res = make([]models.MDBCluster, 0, len(cls))
			for _, cl := range cls {
				res = append(res, models.MDBCluster{
					ClusterExtended: cl,
				})
			}
			return nil
		},
	); err != nil {
		return nil, err
	}

	return res, nil
}

func (ms *Metastore) DeleteCluster(ctx context.Context, cid string) (operations.Operation, error) {
	nodeGroupIDs := make([]string, 0)

	_, err := ms.operator.ModifyOnCluster(ctx, cid, clusters.TypeMetastore,
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

	return ms.operator.Delete(ctx, cid, clusters.TypeMetastore,
		func(ctx context.Context, session sessions.Session, cluster clusterslogic.Cluster, _ clusterslogic.Reader) (operations.Operation, error) {

			opts := taskslogic.DeleteTaskArgs(map[string]interface{}{
				"node_group_ids": nodeGroupIDs,
			})

			op, err := ms.tasks.DeleteCluster(
				ctx,
				session,
				cluster.ClusterID,
				cluster.Revision,
				taskslogic.DeleteClusterTaskTypes{
					Delete:   models.TaskTypeClusterDelete,
					Metadata: models.TaskTypeClusterDeleteMetadata,
				},
				models.OperationTypeClusterDelete,
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
