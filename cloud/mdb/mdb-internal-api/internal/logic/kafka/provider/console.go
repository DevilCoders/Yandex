package provider

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/kafka/provider/internal/kfpillars"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
)

func (kf *Kafka) Version(ctx context.Context, metadb metadb.Backend, cid string) (string, error) {
	cluster, err := metadb.ClusterByClusterID(ctx, cid, models.VisibilityVisible)
	if err != nil {
		return "", err
	}
	var pillar kfpillars.Cluster
	if err := pillar.UnmarshalPillar(cluster.Pillar); err != nil {
		return "", err
	}

	return pillar.Data.Kafka.Version, nil
}
