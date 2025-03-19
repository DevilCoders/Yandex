package hostname

import (
	"fmt"

	"a.yandex-team.ru/cloud/mdb/internal/compute"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/environment"
)

// DataCloudHostnameGenerator build fqdn in format like ych-rc1a-s1-1.mdba4qze3ny2qoeqanm.yadc.io
type DataCloudHostnameGenerator struct{}

func NewDataCloudHostnameGenerator() ClusterHostnameGenerator {
	return DataCloudHostnameGenerator{}
}

var _ ClusterHostnameGenerator = &DataCloudHostnameGenerator{}

func (g DataCloudHostnameGenerator) GenerateFQDN(cloudType environment.CloudType, clusterType clusters.Type, geoName string, shard string, id int, cid string,
	domain string, _ compute.Platform) (string, error) {
	if shard == "" {
		return fmt.Sprintf("%s%s-%s-%d.%s.%s", cloudType.PrefixFQDN(), clusterType.ShortStringified(), geoName, id, cid, domain), nil
	}
	return fmt.Sprintf("%s%s-%s-%s-%d.%s.%s", cloudType.PrefixFQDN(), clusterType.ShortStringified(), geoName, shard, id, cid, domain), nil
}
