package provider

import (
	"context"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/sqlerrors"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	cmodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/console"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (c *Clusters) RevisionByClusterIDAtTime(ctx context.Context, cid string, t time.Time) (int64, error) {
	rev, err := c.metaDB.ClusterRevisionByTime(ctx, cid, t)
	if xerrors.Is(err, sqlerrors.ErrNotFound) {
		err = semerr.WrapWithNotFoundf(err, "cluster %q didn't exist at %q", cid, t.UTC().Format(time.RFC3339))
	}
	return rev, err
}

func (c *Clusters) ResourcesByClusterIDRoleAtRevision(ctx context.Context, cid string, rev int64, role hosts.Role) (models.ClusterResources, error) {
	hosts, err := c.metaDB.HostsByClusterIDRoleAtRevision(ctx, cid, role, rev)
	if err != nil {
		return models.ClusterResources{}, err
	}
	if len(hosts) == 0 {
		return models.ClusterResources{}, xerrors.New("cluster have no hosts")
	}
	host := hosts[0]
	return models.ClusterResources{
		ResourcePresetExtID: host.ResourcePresetExtID,
		DiskSize:            host.SpaceLimit,
		DiskTypeExtID:       host.DiskTypeExtID,
	}, nil
}

func (c *Clusters) ClusterAndResourcesAtTime(ctx context.Context, cid string, t time.Time, typ cmodels.Type, role hosts.Role) (clusters.Cluster, models.ClusterResources, error) {
	rev, err := c.RevisionByClusterIDAtTime(ctx, cid, t)
	if err != nil {
		return clusters.Cluster{}, models.ClusterResources{}, err
	}

	cluster, err := c.ClusterByClusterIDAtRevision(ctx, cid, typ, rev)
	if err != nil {
		return clusters.Cluster{}, models.ClusterResources{}, err
	}

	res, err := c.ResourcesByClusterIDRoleAtRevision(ctx, cid, rev, role)
	if err != nil {
		return clusters.Cluster{}, models.ClusterResources{}, err
	}

	return cluster, res, nil
}

func (c *Clusters) ClusterVersionsAtTime(ctx context.Context, cid string, t time.Time) ([]console.Version, error) {
	rev, err := c.RevisionByClusterIDAtTime(ctx, cid, t)
	if err != nil {
		return []console.Version{}, err
	}

	versions, err := c.ClusterVersionsAtRevision(ctx, cid, rev)
	if err != nil {
		return []console.Version{}, err
	}

	return versions, nil
}

func (c *Clusters) ClusterAndShardResourcesAtTime(ctx context.Context, cid string, time time.Time, typ cmodels.Type, shardName string) (clusters.Cluster, models.ClusterResources, error) {
	rev, err := c.RevisionByClusterIDAtTime(ctx, cid, time)
	if err != nil {
		return clusters.Cluster{}, models.ClusterResources{}, err
	}

	cluster, err := c.ClusterByClusterIDAtRevision(ctx, cid, typ, rev)
	if err != nil {
		return clusters.Cluster{}, models.ClusterResources{}, err
	}

	shardHosts, err := c.metaDB.HostsByClusterIDShardNameAtRevision(ctx, cid, shardName, rev)
	if err != nil {
		return clusters.Cluster{}, models.ClusterResources{}, err
	}
	if len(shardHosts) == 0 {
		return clusters.Cluster{}, models.ClusterResources{}, xerrors.New("shard have no hosts")
	}
	host := shardHosts[0]
	return cluster, models.ClusterResources{
		ResourcePresetExtID: host.ResourcePresetExtID,
		DiskSize:            host.SpaceLimit,
		DiskTypeExtID:       host.DiskTypeExtID,
	}, nil
}

func (c *Clusters) ShardResourcesAtRevision(ctx context.Context, shardName, clusterID string, rev int64) (models.ClusterResources, error) {
	shardHosts, err := c.metaDB.HostsByClusterIDShardNameAtRevision(ctx, clusterID, shardName, rev)
	if err != nil {
		return models.ClusterResources{}, err
	}
	if len(shardHosts) == 0 {
		return models.ClusterResources{}, xerrors.New("shard have no hosts")
	}
	host := shardHosts[0]
	return models.ClusterResources{
		ResourcePresetExtID: host.ResourcePresetExtID,
		DiskSize:            host.SpaceLimit,
		DiskTypeExtID:       host.DiskTypeExtID,
	}, nil
}
