package provider

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/pillars"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (c *Clusters) CreateSubCluster(ctx context.Context, args models.CreateSubClusterArgs) (clusters.SubCluster, error) {
	// TODO: add validation

	// Generate subcluster id if needed
	if args.SubClusterID == "" {
		id, err := c.subClusterIDGenerator.Generate()
		if err != nil {
			return clusters.SubCluster{}, xerrors.Errorf("subcluster id not generated: %w", err)
		}

		args.SubClusterID = id
	}

	sc, err := c.metaDB.CreateSubCluster(ctx, args)
	if err != nil {
		return clusters.SubCluster{}, xerrors.Errorf("failed to add a sub-cluster to cluster %s : %w", args.ClusterID, err)
	}

	return subClusterFromMetaDB(sc), nil
}

func (c *Clusters) CreateKubernetesSubCluster(ctx context.Context, args models.CreateSubClusterArgs) (clusters.SubCluster, error) {
	// TODO: add validation

	// Generate subcluster id if needed
	if args.SubClusterID == "" {
		id, err := c.subClusterIDGenerator.Generate()
		if err != nil {
			return clusters.SubCluster{}, xerrors.Errorf("subcluster id not generated: %w", err)
		}

		args.SubClusterID = id
	}

	sc, err := c.metaDB.CreateKubernetesSubCluster(ctx, args)
	if err != nil {
		return clusters.SubCluster{}, xerrors.Errorf("failed to add a sub-cluster to cluster %s : %w", args.ClusterID, err)
	}

	return subClusterFromMetaDB(sc), nil
}

func (c *Clusters) subClusterByRole(ctx context.Context, cid string, role hosts.Role, rev optional.Int64) (metadb.SubCluster, error) {
	// Load all known subcluster pillars
	var err error
	var scs []metadb.SubCluster
	if !rev.Valid {
		scs, err = c.metaDB.SubClustersByClusterID(ctx, cid)
	} else {
		scs, err = c.metaDB.SubClustersByClusterIDAtRevision(ctx, cid, rev.Int64)
	}
	if err != nil {
		return metadb.SubCluster{}, xerrors.Errorf("failed to retrieve subcluster by cid %q: %w", cid, err)
	}

	// Find the right one
	for _, sc := range scs {
		for _, r := range sc.Roles {
			if role == r {
				return sc, nil
			}
		}
	}

	return metadb.SubCluster{}, xerrors.Errorf("failed to retrieve subcluster by cid %q and role %q", cid, role)
}

// SubClusterByRole accepts pillar in its marshaler form. If subcluster is found, pillar will be unmarshaled
// using provided marshaler.
func (c *Clusters) SubClusterByRole(ctx context.Context, cid string, role hosts.Role, pillar pillars.Marshaler) (clusters.SubCluster, error) {
	sc, err := c.subClusterByRole(ctx, cid, role, optional.Int64{})
	if err != nil {
		return clusters.SubCluster{}, err
	}

	if err = pillar.UnmarshalPillar(sc.Pillar); err != nil {
		return clusters.SubCluster{}, err
	}

	return subClusterFromMetaDB(sc), nil
}

func (c *Clusters) SubClusterByRoleAtRevision(ctx context.Context, cid string, role hosts.Role, pillar pillars.Marshaler, rev int64) (clusters.SubCluster, error) {
	sc, err := c.subClusterByRole(ctx, cid, role, optional.NewInt64(rev))
	if err != nil {
		return clusters.SubCluster{}, err
	}

	if err = pillar.UnmarshalPillar(sc.Pillar); err != nil {
		return clusters.SubCluster{}, err
	}

	return subClusterFromMetaDB(sc), nil
}

func (c *Clusters) KubernetesSubClusters(ctx context.Context, cid string) ([]clusters.KubernetesSubCluster, error) {
	subClusters, err := c.metaDB.SubClustersByClusterID(ctx, cid)
	if err != nil {
		return []clusters.KubernetesSubCluster{}, err
	}
	kubernetesSubClusters := make([]clusters.KubernetesSubCluster, 0)
	for _, subCluster := range subClusters {
		nodeGroup, err := c.metaDB.KubernetesNodeGroup(ctx, subCluster.SubClusterID)
		if err != nil {
			return []clusters.KubernetesSubCluster{}, err
		}
		kubernetesSubClusters = append(
			kubernetesSubClusters,
			clusters.KubernetesSubCluster{
				SubCluster: clusters.SubCluster{
					SubClusterID: subCluster.SubClusterID,
					ClusterID:    subCluster.ClusterID,
					Name:         subCluster.Name,
				},
				KubernetesClusterID: nodeGroup.KubernetesClusterID,
				NodeGroupID:         nodeGroup.NodeGroupID,
			},
		)
	}
	return kubernetesSubClusters, nil
}

func subClusterFromMetaDB(from metadb.SubCluster) clusters.SubCluster {
	return from.SubCluster
}

func (c *Clusters) DeleteSubCluster(ctx context.Context, cid, subcid string, revision int64) error {
	err := c.metaDB.DeleteSubCluster(ctx, models.DeleteSubClusterArgs{ClusterID: cid, SubClusterID: subcid, Revision: revision})
	if err != nil {
		return xerrors.Errorf("failed to delete subcluster from metadb: %w", err)
	}

	return nil
}
