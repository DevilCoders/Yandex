package provider

import (
	"encoding/json"

	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/metastore/provider/internal/pillars"
)

func getPillarFromJSON(jsonRawPillar json.RawMessage) (*pillars.Cluster, error) {
	pillar := pillars.NewCluster()
	if err := pillar.UnmarshalPillar(jsonRawPillar); err != nil {
		return nil, err
	}
	return pillar, nil
}
