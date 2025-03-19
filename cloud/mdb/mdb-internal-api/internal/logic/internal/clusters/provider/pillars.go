package provider

import (
	"context"
	"encoding/json"

	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/clusters"
	clmodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/pillars"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (c *Clusters) AddClusterPillar(ctx context.Context, cid string, revision int64, pillar clusters.Pillar) error {
	if err := pillar.Validate(); err != nil {
		return err
	}

	marshaled, err := json.Marshal(pillar)
	if err != nil {
		return xerrors.Errorf("failed to marshal cluster pillar to raw form: %w", err)
	}

	if err = c.metaDB.AddClusterPillar(ctx, cid, revision, marshaled); err != nil {
		return xerrors.Errorf("failed to update cluster pillar in metadb: %w", err)
	}
	return nil
}

func (c *Clusters) UpdatePillar(ctx context.Context, cid string, revision int64, pillar clusters.Pillar) error {
	if err := pillar.Validate(); err != nil {
		return err
	}
	marshaled, err := json.Marshal(pillar)
	if err != nil {
		return xerrors.Errorf("failed to marshal cluster pillar to raw form: %w", err)
	}

	if err = c.metaDB.UpdateClusterPillar(ctx, cid, revision, marshaled); err != nil {
		return xerrors.Errorf("failed to update cluster pillar in metadb: %w", err)
	}

	return nil
}

func (c *Clusters) AddSubClusterPillar(ctx context.Context, cid, subcid string, revision int64, pillar pillars.Marshaler) error {
	marshaled, err := pillar.MarshalPillar()
	if err != nil {
		return xerrors.Errorf("failed to marshal subcluster pillar to raw form: %w", err)
	}

	if err = c.metaDB.AddSubClusterPillar(ctx, cid, subcid, revision, marshaled); err != nil {
		return xerrors.Errorf("failed to add subcluster pillar to metadb: %w", err)
	}

	return nil
}

func (c *Clusters) AddShardPillar(ctx context.Context, cid, shardid string, revision int64, pillar pillars.Marshaler) error {
	marshaled, err := pillar.MarshalPillar()
	if err != nil {
		return xerrors.Errorf("failed to marshal shard pillar to raw form: %w", err)
	}

	if err = c.metaDB.AddShardPillar(ctx, cid, shardid, revision, marshaled); err != nil {
		return xerrors.Errorf("failed to add shard pillar to metadb: %w", err)
	}

	return nil
}

func (c *Clusters) AddTargetPillar(ctx context.Context, cid string, pillar pillars.Marshaler) (string, error) {
	targetPillarID, err := c.pillarIDGenerator.Generate()
	if err != nil {
		return "", xerrors.Errorf("failed to generate pillar id: %w", err)
	}
	marshaled, err := pillar.MarshalPillar()
	if err != nil {
		return "", xerrors.Errorf("failed to marshal subcluster pillar to raw form: %w", err)
	}
	err = c.metaDB.AddTargetPillar(ctx, targetPillarID, marshaled, map[string]interface{}{"pillar_cid": cid})
	if err != nil {
		return "", xerrors.Errorf("failed to add subcluster pillar to metadb: %w", err)
	}
	return targetPillarID, nil
}

func (c *Clusters) UpdateSubClusterPillar(ctx context.Context, cid, subcid string, revision int64, pillar pillars.Marshaler) error {
	marshaled, err := pillar.MarshalPillar()
	if err != nil {
		return xerrors.Errorf("failed to marshal subcluster pillar to raw form: %w", err)
	}

	if err = c.metaDB.UpdateSubClusterPillar(ctx, cid, subcid, revision, marshaled); err != nil {
		return xerrors.Errorf("failed to update subcluster pillar in metadb: %w", err)
	}

	return nil
}

func (c *Clusters) ClusterTypePillar(ctx context.Context, typ clmodels.Type, marshaller pillars.Marshaler) error {
	return c.metaDB.ClusterTypePillar(ctx, typ, marshaller)
}

func (c *Clusters) AddHostPillar(ctx context.Context, cid string, fqdn string, revision int64, marshaller pillars.Marshaler) error {
	return c.metaDB.AddHostPillar(
		ctx,
		cid,
		fqdn,
		revision,
		marshaller,
	)
}

func (c *Clusters) UpdateHostPillar(ctx context.Context, cid string, fqdn string, revision int64, marshaller pillars.Marshaler) error {
	return c.metaDB.UpdateHostPillar(
		ctx,
		cid,
		fqdn,
		revision,
		marshaller,
	)
}

func (c *Clusters) HostPillar(ctx context.Context, fqdn string, marshaller pillars.Marshaler) error {
	return c.metaDB.HostPillar(ctx, fqdn, marshaller)
}

func (c *Clusters) UpdateShardPillar(ctx context.Context, cid, shardid string, revision int64, pillar pillars.Marshaler) error {
	marshaled, err := pillar.MarshalPillar()
	if err != nil {
		return xerrors.Errorf("failed to marshal shard pillar to raw form: %w", err)
	}

	if err = c.metaDB.UpdateShardPillar(ctx, cid, shardid, revision, marshaled); err != nil {
		return xerrors.Errorf("failed to update subcluster pillar in metadb: %w", err)
	}

	return nil
}
