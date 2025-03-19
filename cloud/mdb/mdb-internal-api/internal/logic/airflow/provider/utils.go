package provider

import (
	"encoding/json"

	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/airflow/provider/internal/afpillars"
	clusterslogic "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/clusters"
)

func getPillarFromCluster(cluster clusterslogic.Cluster) (*afpillars.Cluster, error) {
	pillar := afpillars.NewCluster()
	if err := cluster.Pillar(pillar); err != nil {
		return nil, err
	}

	return pillar, nil
}

func getPillarFromJSON(jsonRawPillar json.RawMessage) (*afpillars.Cluster, error) {
	pillar := afpillars.NewCluster()
	if err := pillar.UnmarshalPillar(jsonRawPillar); err != nil {
		return nil, err
	}
	return pillar, nil
}
