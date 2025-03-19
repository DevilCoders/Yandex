package provider

import (
	"context"
	"encoding/json"

	clusterslogic "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/sessions"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/kafka/provider/internal/kfpillars"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/environment"
	"a.yandex-team.ru/library/go/x/math"
)

type creatorModifier interface {
	SelectZonesForCloudAndRegion(ctx context.Context, session sessions.Session,
		cloudType environment.CloudType, regionID string, forceFilterDecommissionedZone bool, zoneCount int) ([]string, error)
}

func getPillarFromCluster(cluster clusterslogic.Cluster) (*kfpillars.Cluster, error) {
	pillar := kfpillars.NewCluster()
	if err := cluster.Pillar(pillar); err != nil {
		return nil, err
	}

	return pillar, nil
}

func getPillarFromJSON(jsonRawPillar json.RawMessage) (*kfpillars.Cluster, error) {
	pillar := kfpillars.NewCluster()
	if err := pillar.UnmarshalPillar(jsonRawPillar); err != nil {
		return nil, err
	}
	return pillar, nil
}

func selectDataCloudKafkaAndZookeeperHostZones(ctx context.Context, session sessions.Session, modifier creatorModifier,
	cloudType environment.CloudType, regionID string, zoneCount int64, oneHostMode bool) ([]string, []string, error) {
	const (
		defaultZookeeperClusterZones = 3
	)

	var zoneNumber, zkZonesNumber int
	if oneHostMode {
		zoneNumber = int(zoneCount)
		zkZonesNumber = 0
	} else {
		// If we need zk cluster then we need at least 3 AZ.
		zoneNumber = math.MaxInt(int(zoneCount), defaultZookeeperClusterZones)
		zkZonesNumber = defaultZookeeperClusterZones
	}

	availableZones, err := modifier.SelectZonesForCloudAndRegion(ctx, session, cloudType, regionID, true, zoneNumber)
	if err != nil {
		return nil, nil, err
	}

	kafkaZones := availableZones[0:zoneCount]
	zkZones := availableZones[0:zkZonesNumber]

	return kafkaZones, zkZones, nil
}

func (kf *Kafka) IsEnvPrestable(env environment.SaltEnv) bool {
	return kf.cfg.SaltEnvs.Prestable == env
}
