package provider

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/metastore/provider/internal/pillars"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
)

func (ms *Metastore) Version(ctx context.Context, metadb metadb.Backend, cid string) (string, error) {
	cluster, err := metadb.ClusterByClusterID(ctx, cid, models.VisibilityVisible)
	if err != nil {
		return "", err
	}
	var pillar pillars.Cluster
	if err := pillar.UnmarshalPillar(cluster.Pillar); err != nil {
		return "", err
	}

	return pillar.Data.Version, nil
}
