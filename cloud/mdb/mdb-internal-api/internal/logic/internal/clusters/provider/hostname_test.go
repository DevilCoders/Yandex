package provider

import (
	"fmt"
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/compute"
	"a.yandex-team.ru/cloud/mdb/internal/generator"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/clusters/provider/hostname"
	clustermodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/environment"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/resources"
)

type TestHostnameGenerator struct {
	Counter int64
}

func (h *TestHostnameGenerator) Generate(prefix, suffix string) (string, error) {
	res := prefix + fmt.Sprintf("hostname%d", h.Counter) + suffix
	h.Counter++
	return res, nil
}

var _ generator.HostnameGenerator = &TestHostnameGenerator{}

func getMDBPlatformTestHostnameGenerator() hostname.ClusterHostnameGenerator {
	return hostname.NewMdbHostnameGenerator(generator.NewPlatformTestHostnameGenerator(map[compute.Platform]generator.HostnameGenerator{
		compute.Ubuntu: &TestHostnameGenerator{},
	}))
}

func TestGenerateMdbFQDN(t *testing.T) {
	config := logic.Config{
		VTypes: map[environment.VType]string{
			environment.VTypeCompute: "compute.yandex.net",
			environment.VTypePorto:   "porto.yandex.net",
		},
		ZoneRenameMap: map[string]string{
			"vla": "vla-mapped",
			"sas": "sas-mapped",
		},
	}
	clusters := NewClusters(nil, nil, nil, nil, nil, getMDBPlatformTestHostnameGenerator(), nil, nil, nil, config, nil)
	data := []struct {
		inputZone  string
		inputVType environment.VType
		expected   string
	}{
		{
			inputZone:  "vla",
			inputVType: environment.VTypeCompute,
			expected:   "vla-mapped-hostname0.compute.yandex.net",
		},
		{
			inputZone:  "sas",
			inputVType: environment.VTypeCompute,
			expected:   "sas-mapped-hostname1.compute.yandex.net",
		},
		{
			inputZone:  "myt",
			inputVType: environment.VTypeCompute,
			expected:   "myt-hostname2.compute.yandex.net",
		},
		{
			inputZone:  "myt",
			inputVType: environment.VTypePorto,
			expected:   "myt-hostname3.porto.yandex.net",
		},
		{
			inputZone:  "sas",
			inputVType: environment.VTypePorto,
			expected:   "sas-mapped-hostname4.porto.yandex.net",
		},
	}

	for _, d := range data {
		fqdn, err := clusters.GenerateFQDN(d.inputZone, resources.Preset{VType: d.inputVType}.VType, compute.Ubuntu)
		require.NoError(t, err)
		require.Equal(t, d.expected, fqdn)
	}
}

func TestGenerateMdbFQDNEmptyZone(t *testing.T) {
	config := logic.Config{
		VTypes: map[environment.VType]string{
			environment.VTypeCompute: "yandex.net",
		},
		ZoneRenameMap: map[string]string{},
	}
	preset := resources.Preset{VType: environment.VTypeCompute}
	clusters := NewClusters(nil, nil, nil, nil, nil, getMDBPlatformTestHostnameGenerator(), nil, nil, nil, config, nil)
	_, err := clusters.GenerateFQDN("", preset.VType, compute.Ubuntu)

	require.Error(t, err)
}

func TestGenerateMdbFQDNVTypeNotFound(t *testing.T) {
	config := logic.Config{
		VTypes: map[environment.VType]string{
			environment.VTypeCompute: "yandex.net",
		},
		ZoneRenameMap: map[string]string{},
	}
	preset := resources.Preset{VType: environment.VTypePorto}
	clusters := NewClusters(nil, nil, nil, nil, nil, getMDBPlatformTestHostnameGenerator(), nil, nil, nil, config, nil)
	_, err := clusters.GenerateFQDN("vla", preset.VType, compute.Ubuntu)

	require.Error(t, err)
}

func TestSortCreateZones(t *testing.T) {
	t.Run("Add third zone and new host in existing", func(t *testing.T) {
		toCreate := clusters.ZoneHostsList{{ZoneID: "A", Count: 1}, {ZoneID: "B", Count: 1}}
		sortCreateZones(toCreate, clusters.ZoneHostsList{{ZoneID: "C", Count: 1}, {ZoneID: "A", Count: 1}})
		require.Equal(t, clusters.ZoneHostsList{{ZoneID: "B", Count: 1}, {ZoneID: "A", Count: 1}}, toCreate)
	})

	t.Run("Add replicas in all zones", func(t *testing.T) {
		toCreate := clusters.ZoneHostsList{{ZoneID: "A", Count: 1}, {ZoneID: "B", Count: 1}, {ZoneID: "C", Count: 1}}
		sortCreateZones(toCreate, clusters.ZoneHostsList{{ZoneID: "A", Count: 1}, {ZoneID: "B", Count: 1}})
		require.Equal(t, clusters.ZoneHostsList{{ZoneID: "C", Count: 1}, {ZoneID: "A", Count: 1}, {ZoneID: "B", Count: 1}}, toCreate)
	})
}

func TestGenerateSemanticFQDNs(t *testing.T) {
	config := logic.Config{
		VTypes:        map[environment.VType]string{environment.VTypeAWS: "yadc.io"},
		ZoneRenameMap: map[string]string{},
	}
	clustersLogic := NewClusters(nil, nil, nil, nil, nil, hostname.NewDataCloudHostnameGenerator(), nil, nil, nil, config, nil)

	t.Run("Empty", func(t *testing.T) {
		fqdns, err := clustersLogic.GenerateSemanticFQDNs(environment.CloudTypeAWS, clustermodels.TypeClickHouse,
			clusters.ZoneHostsList{},
			clusters.ZoneHostsList{},
			"s1", "cid1", environment.VTypeAWS, compute.Ubuntu)
		require.NoError(t, err)
		require.Equal(t, map[string][]string{}, fqdns)
	})

	t.Run("Create single in all zones", func(t *testing.T) {
		fqdns, err := clustersLogic.GenerateSemanticFQDNs(environment.CloudTypeAWS, clustermodels.TypeClickHouse,
			clusters.ZoneHostsList{{ZoneID: "rc1b", Count: 1}, {ZoneID: "rc1c", Count: 1}, {ZoneID: "rc1a", Count: 1}},
			clusters.ZoneHostsList{},
			"s1", "cid1", environment.VTypeAWS, compute.Ubuntu)
		require.NoError(t, err)
		require.Equal(t, map[string][]string{
			"rc1a": {"ach-rc1a-s1-1.cid1.yadc.io"},
			"rc1b": {"ach-rc1b-s1-2.cid1.yadc.io"},
			"rc1c": {"ach-rc1c-s1-3.cid1.yadc.io"},
		}, fqdns)
	})

	t.Run("Create multiple in all zones", func(t *testing.T) {
		fqdns, err := clustersLogic.GenerateSemanticFQDNs(environment.CloudTypeAWS, clustermodels.TypeClickHouse,
			clusters.ZoneHostsList{{ZoneID: "rc1b", Count: 2}, {ZoneID: "rc1c", Count: 2}, {ZoneID: "rc1a", Count: 2}},
			clusters.ZoneHostsList{},
			"s1", "cid1", environment.VTypeAWS, compute.Ubuntu)
		require.NoError(t, err)
		require.Equal(t, map[string][]string{
			"rc1a": {"ach-rc1a-s1-1.cid1.yadc.io", "ach-rc1a-s1-4.cid1.yadc.io"},
			"rc1b": {"ach-rc1b-s1-2.cid1.yadc.io", "ach-rc1b-s1-5.cid1.yadc.io"},
			"rc1c": {"ach-rc1c-s1-3.cid1.yadc.io", "ach-rc1c-s1-6.cid1.yadc.io"},
		}, fqdns)
	})

	t.Run("Add new zone", func(t *testing.T) {
		fqdns, err := clustersLogic.GenerateSemanticFQDNs(environment.CloudTypeAWS, clustermodels.TypeClickHouse,
			clusters.ZoneHostsList{{ZoneID: "rc1b", Count: 1}},
			clusters.ZoneHostsList{{ZoneID: "rc1c", Count: 1}, {ZoneID: "rc1a", Count: 1}},
			"s1", "cid1", environment.VTypeAWS, compute.Ubuntu)
		require.NoError(t, err)
		require.Equal(t, map[string][]string{
			"rc1b": {"ach-rc1b-s1-3.cid1.yadc.io"},
		}, fqdns)
	})

	t.Run("Add new zone and some hosts", func(t *testing.T) {
		fqdns, err := clustersLogic.GenerateSemanticFQDNs(environment.CloudTypeAWS, clustermodels.TypeClickHouse,
			clusters.ZoneHostsList{{ZoneID: "rc1b", Count: 1}, {ZoneID: "rc1c", Count: 1}, {ZoneID: "rc1a", Count: 1}},
			clusters.ZoneHostsList{{ZoneID: "rc1c", Count: 1}, {ZoneID: "rc1a", Count: 1}},
			"s1", "cid1", environment.VTypeAWS, compute.Ubuntu)
		require.NoError(t, err)
		require.Equal(t, map[string][]string{
			"rc1a": {"ach-rc1a-s1-4.cid1.yadc.io"},
			"rc1b": {"ach-rc1b-s1-3.cid1.yadc.io"},
			"rc1c": {"ach-rc1c-s1-5.cid1.yadc.io"},
		}, fqdns)
	})

	t.Run("Add two new zones", func(t *testing.T) {
		fqdns, err := clustersLogic.GenerateSemanticFQDNs(environment.CloudTypeAWS, clustermodels.TypeClickHouse,
			clusters.ZoneHostsList{{ZoneID: "rc1c", Count: 1}, {ZoneID: "rc1b", Count: 1}},
			clusters.ZoneHostsList{{ZoneID: "rc1a", Count: 1}},
			"s1", "cid1", environment.VTypeAWS, compute.Ubuntu)
		require.NoError(t, err)
		require.Equal(t, map[string][]string{
			"rc1b": {"ach-rc1b-s1-2.cid1.yadc.io"},
			"rc1c": {"ach-rc1c-s1-3.cid1.yadc.io"},
		}, fqdns)
	})

	t.Run("Add multiple in new zone", func(t *testing.T) {
		fqdns, err := clustersLogic.GenerateSemanticFQDNs(environment.CloudTypeAWS, clustermodels.TypeClickHouse,
			clusters.ZoneHostsList{{ZoneID: "rc1c", Count: 1}, {ZoneID: "rc1b", Count: 2}},
			clusters.ZoneHostsList{{ZoneID: "rc1a", Count: 1}},
			"s1", "cid1", environment.VTypeAWS, compute.Ubuntu)
		require.NoError(t, err)
		require.Equal(t, map[string][]string{
			"rc1b": {"ach-rc1b-s1-2.cid1.yadc.io", "ach-rc1b-s1-4.cid1.yadc.io"},
			"rc1c": {"ach-rc1c-s1-3.cid1.yadc.io"},
		}, fqdns)
	})

	t.Run("Add different in zones", func(t *testing.T) {
		fqdns, err := clustersLogic.GenerateSemanticFQDNs(environment.CloudTypeAWS, clustermodels.TypeClickHouse,
			clusters.ZoneHostsList{{ZoneID: "rc1c", Count: 2}, {ZoneID: "rc1b", Count: 2}, {ZoneID: "rc1a", Count: 1}},
			clusters.ZoneHostsList{},
			"s1", "cid1", environment.VTypeAWS, compute.Ubuntu)
		require.NoError(t, err)
		require.Equal(t, map[string][]string{
			"rc1a": {"ach-rc1a-s1-1.cid1.yadc.io"},
			"rc1b": {"ach-rc1b-s1-2.cid1.yadc.io", "ach-rc1b-s1-4.cid1.yadc.io"},
			"rc1c": {"ach-rc1c-s1-3.cid1.yadc.io", "ach-rc1c-s1-5.cid1.yadc.io"},
		}, fqdns)
	})
}
