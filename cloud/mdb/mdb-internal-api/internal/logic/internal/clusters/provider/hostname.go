package provider

import (
	"sort"

	"a.yandex-team.ru/cloud/mdb/internal/compute"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/clusters"
	clustermodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/environment"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (c *Clusters) GenerateFQDN(geoName string, vtype environment.VType, platform compute.Platform) (string, error) {
	if geoName == "" {
		return "", xerrors.New("geo must be specified")
	}

	fqdns, err := c.GenerateSemanticFQDNs(
		environment.CloudTypeYandex,
		clustermodels.TypeUnknown,
		clusters.ZoneHostsList{clusters.ZoneHosts{ZoneID: geoName, Count: 1}},
		nil, "", "", vtype, platform,
	)
	if err != nil {
		return "", err
	}

	return fqdns[geoName][0], nil
}

func (c *Clusters) GenerateSemanticFQDNs(
	cloudType environment.CloudType,
	clusterType clustermodels.Type,
	zonesToCreate clusters.ZoneHostsList,
	zonesCurrent clusters.ZoneHostsList,
	shardName string,
	cid string,
	vtype environment.VType,
	platform compute.Platform,
) (map[string][]string, error) {
	result := map[string][]string{}
	suffix, err := c.cfg.GetDomain(vtype)
	if err != nil {
		return nil, err
	}
	indexDiff := int(zonesCurrent.Len())

	sortCreateZones(zonesToCreate, zonesCurrent)
	targetZoneIndex := 0
	for i := 0; i < int(zonesToCreate.Len()); i += 1 {
		for zonesToCreate[targetZoneIndex].Count == int64(len(result[zonesToCreate[targetZoneIndex].ZoneID])) {
			targetZoneIndex = (targetZoneIndex + 1) % len(zonesToCreate)
		}

		fqdn, err := c.hostnameGenerator.GenerateFQDN(
			cloudType,
			clusterType, c.cfg.MapZoneName(zonesToCreate[targetZoneIndex].ZoneID),
			shardName,
			i+indexDiff+1,
			cid,
			suffix,
			platform,
		)
		if err != nil {
			return nil, err
		}

		result[zonesToCreate[targetZoneIndex].ZoneID] = append(result[zonesToCreate[targetZoneIndex].ZoneID], fqdn)
		targetZoneIndex = (targetZoneIndex + 1) % len(zonesToCreate)
	}

	return result, nil
}

func sortCreateZones(zonesToCreate, zonesCurrent clusters.ZoneHostsList) {
	currentZoneMap := zonesCurrent.ToMap()

	sort.Slice(zonesToCreate, func(i, j int) bool {
		if currentZoneMap[zonesToCreate[i].ZoneID] == currentZoneMap[zonesToCreate[j].ZoneID] {
			return zonesToCreate[i].ZoneID < zonesToCreate[j].ZoneID
		}
		return currentZoneMap[zonesToCreate[i].ZoneID] < currentZoneMap[zonesToCreate[j].ZoneID]
	})
}
