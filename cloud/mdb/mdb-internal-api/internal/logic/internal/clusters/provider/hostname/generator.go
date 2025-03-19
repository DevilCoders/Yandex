package hostname

import (
	"a.yandex-team.ru/cloud/mdb/internal/compute"
	clustermodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/environment"
)

type ClusterHostnameGenerator interface {
	GenerateFQDN(cloudType environment.CloudType, clusterType clustermodels.Type, geoName string, shard string, id int, cid string,
		domain string, platform compute.Platform) (string, error)
}
