package provider

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/internal/sentry"
	clusterslogic "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts/services"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/pillars"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func getShardImpl(shard metadb.Shard, pillar pillars.Marshaler) (clusters.Shard, error) {
	err := pillar.UnmarshalPillar(shard.Pillar)
	if err != nil {
		return clusters.Shard{}, err
	}

	return shardFromMetaDB(shard), nil
}

func (c *Clusters) ShardByShardID(ctx context.Context, shardID string, pillar pillars.Marshaler) (clusters.Shard, error) {
	shard, err := c.metaDB.ShardByShardID(ctx, shardID)
	if err != nil {
		return clusters.Shard{}, err
	}

	return getShardImpl(shard, pillar)
}

func (c *Clusters) ShardByShardName(ctx context.Context, shardName, clusterID string, pillar pillars.Marshaler) (clusters.Shard, error) {
	shardMeta, err := c.metaDB.ShardByShardName(ctx, shardName, clusterID)
	if err != nil {
		return clusters.Shard{}, err
	}

	return getShardImpl(shardMeta, pillar)
}

func (c *Clusters) ListShards(ctx context.Context, clusterID string) ([]clusters.Shard, error) {
	shards, err := c.metaDB.ShardsByClusterID(ctx, clusterID)
	if err != nil {
		return []clusters.Shard{}, xerrors.Errorf("failed to retrieve shards by cid %q: %w", clusterID, err)
	}

	res := make([]clusters.Shard, 0, len(shards))
	for _, shard := range shards {
		res = append(res, shardFromMetaDB(shard))
	}
	return res, nil
}

func (c *Clusters) ListShardsExtended(ctx context.Context, clusterID string) ([]clusters.ShardExtended, error) {
	shards, err := c.metaDB.ShardsByClusterID(ctx, clusterID)
	if err != nil {
		return nil, xerrors.Errorf("failed to retrieve shards by cid %q: %w", clusterID, err)
	}

	res := make([]clusters.ShardExtended, 0, len(shards))
	for _, s := range shards {
		res = append(res, shardExtendedFromMetaDB(s))
	}

	return res, nil
}

func (c *Clusters) ListShardHosts(ctx context.Context, shardID, clusterID string) ([]hosts.HostExtended, error) {
	loadedHosts, err := c.metaDB.HostsByShardID(ctx, shardID, clusterID)
	if err != nil {
		return nil, xerrors.Errorf("failed to retrieve shard %q hosts: %w", shardID, err)
	}

	var fqdns []string
	for _, host := range loadedHosts {
		fqdns = append(fqdns, host.FQDN)
	}

	// Load health
	healths, err := c.health.Hosts(ctx, fqdns)
	if err != nil {
		c.l.Error("cannot retrieve hosts health")
		sentry.GlobalClient().CaptureError(ctx, err, nil)
	}
	res := make([]hosts.HostExtended, len(loadedHosts))
	for i, host := range loadedHosts {
		health, ok := healths[host.FQDN]
		if !ok {
			health = hosts.Health{
				Status:   hosts.StatusUnknown,
				Services: []services.Health{},
				System:   nil,
			}
		}

		res[i] = hosts.HostExtended{Host: host, Health: health}
	}

	return res, nil
}

func (c *Clusters) CreateShard(ctx context.Context, args models.CreateShardArgs) (clusters.Shard, error) {
	// TODO: add validation

	// Generate shard id if needed
	if args.ShardID == "" {
		id, err := c.shardIDGenerator.Generate()
		if err != nil {
			return clusters.Shard{}, xerrors.Errorf("shard id not generated: %w", err)
		}

		args.ShardID = id
	}

	s, err := c.metaDB.CreateShard(ctx, args)
	if err != nil {
		return clusters.Shard{}, xerrors.Errorf("failed to add a shard to cluster %s : %w", args.ClusterID, err)
	}

	return shardFromMetaDB(s), nil
}

func (c *Clusters) DeleteShard(ctx context.Context, cid, shardID string, revision int64) error {
	clusterHosts, err := clusterslogic.ListAllHosts(ctx, c, cid)
	if err != nil {
		return err
	}

	shardHosts := make([]string, 0, 3)
	for _, host := range clusterHosts {
		if host.ShardID.String == shardID {
			shardHosts = append(shardHosts, host.FQDN)
		}
	}

	if _, err := c.DeleteHosts(ctx, cid, shardHosts, revision); err != nil {
		return err
	}

	return c.metaDB.DeleteShard(ctx, cid, shardID, revision)
}

func shardFromMetaDB(from metadb.Shard) clusters.Shard {
	return from.Shard
}

func shardExtendedFromMetaDB(from metadb.Shard) clusters.ShardExtended {
	return clusters.ShardExtended{
		Shard:  from.Shard,
		Pillar: from.Pillar,
	}
}
