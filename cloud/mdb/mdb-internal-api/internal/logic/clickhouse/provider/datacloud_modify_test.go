package provider

import (
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts"
)

func TestClickHouse_UpdateKeeperHosts(t *testing.T) {
	t.Run("Upscale to 3 node config", func(t *testing.T) {
		keeperFQDNs := []string{"keeper1"}
		allHosts := []hosts.Host{
			{
				FQDN:   "keeper1",
				ZoneID: "zone1",
			},
			{
				FQDN:   "new_keeper_zone1",
				ZoneID: "zone1",
			},
			{
				FQDN:   "new_keeper_zone1",
				ZoneID: "zone2",
			},
			{
				FQDN:   "not_a_keeper",
				ZoneID: "zone2",
			},
		}
		result, oldKeepers, newKeepers := updateKeeperHosts(keeperFQDNs, allHosts)
		require.True(t, result)
		require.ElementsMatch(t, oldKeepers, []hosts.Host{{
			FQDN:   "keeper1",
			ZoneID: "zone1",
		}})

		require.ElementsMatch(t, newKeepers, []hosts.Host{
			{
				FQDN:   "new_keeper_zone1",
				ZoneID: "zone1",
			},
			{
				FQDN:   "new_keeper_zone1",
				ZoneID: "zone2",
			},
		})
	})

	t.Run("Downscale to 1 node config", func(t *testing.T) {
		keeperFQDNs := []string{"keeper1", "keeper2", "keeper3"}
		allHosts := []hosts.Host{
			{
				FQDN:   "keeper1",
				ZoneID: "zone1",
			},
			{
				FQDN:   "not_a_keeper",
				ZoneID: "zone2",
			},
		}
		result, oldKeepers, newKeepers := updateKeeperHosts(keeperFQDNs, allHosts)
		require.True(t, result)
		require.ElementsMatch(t, oldKeepers, []hosts.Host{{
			FQDN:   "keeper1",
			ZoneID: "zone1",
		}})

		require.ElementsMatch(t, newKeepers, []hosts.Host{})
	})

	t.Run("Keepers were removed without downscale, without rebalace", func(t *testing.T) {
		keeperFQDNs := []string{"keeper1", "keeper2", "keeper3"}
		allHosts := []hosts.Host{
			{
				FQDN:   "keeper1",
				ZoneID: "zone1",
			},
			{
				FQDN:   "not_a_keeper",
				ZoneID: "zone1",
			},
			{
				FQDN:   "keeper2",
				ZoneID: "zone2",
			},
			{
				FQDN:   "new_keeper3",
				ZoneID: "zone2",
			},
		}
		result, oldKeepers, newKeepers := updateKeeperHosts(keeperFQDNs, allHosts)
		require.True(t, result)
		require.ElementsMatch(t, oldKeepers, []hosts.Host{
			{
				FQDN:   "keeper1",
				ZoneID: "zone1",
			},
			{
				FQDN:   "keeper2",
				ZoneID: "zone2",
			},
		})

		require.ElementsMatch(t, newKeepers, []hosts.Host{{
			FQDN:   "new_keeper3",
			ZoneID: "zone2",
		}})
	})

	t.Run("Keepers were removed without downscale, without rebalance (2 zones)", func(t *testing.T) {
		keeperFQDNs := []string{"keeper1", "keeper2", "keeper3"}
		allHosts := []hosts.Host{
			{
				FQDN:   "keeper1",
				ZoneID: "zone1",
			},
			{
				FQDN:   "keeper2",
				ZoneID: "zone1",
			},
			{
				FQDN:   "new_keeper3",
				ZoneID: "zone2",
			},
			{
				FQDN:   "not_a_keeper",
				ZoneID: "zone2",
			},
		}
		result, oldKeepers, newKeepers := updateKeeperHosts(keeperFQDNs, allHosts)
		require.True(t, result)
		require.ElementsMatch(t, oldKeepers, []hosts.Host{
			{
				FQDN:   "keeper1",
				ZoneID: "zone1",
			},
			{
				FQDN:   "keeper2",
				ZoneID: "zone1",
			},
		})

		require.ElementsMatch(t, newKeepers, []hosts.Host{{
			FQDN:   "new_keeper3",
			ZoneID: "zone2",
		}})
	})

	t.Run("Keepers were removed without downscale, with rebalance", func(t *testing.T) {
		keeperFQDNs := []string{"keeper1", "keeper2", "keeper3"}
		allHosts := []hosts.Host{
			{
				FQDN:   "keeper1",
				ZoneID: "zone1",
			},
			{
				FQDN:   "keeper2",
				ZoneID: "zone1",
			},
			{
				FQDN:   "new_keeper2",
				ZoneID: "zone2",
			},
			{
				FQDN:   "new_keeper3",
				ZoneID: "zone3",
			},
		}
		result, oldKeepers, newKeepers := updateKeeperHosts(keeperFQDNs, allHosts)
		require.True(t, result)
		require.ElementsMatch(t, oldKeepers, []hosts.Host{
			{
				FQDN:   "keeper1",
				ZoneID: "zone1",
			},
		})

		require.ElementsMatch(t, newKeepers, []hosts.Host{
			{
				FQDN:   "new_keeper2",
				ZoneID: "zone2",
			},
			{
				FQDN:   "new_keeper3",
				ZoneID: "zone3",
			},
		})
	})

	t.Run("Rebalance keepers by zones", func(t *testing.T) {
		keeperFQDNs := []string{"keeper1", "keeper2", "keeper3"}
		allHosts := []hosts.Host{
			{
				FQDN:   "keeper1",
				ZoneID: "zone1",
			},
			{
				FQDN:   "keeper2",
				ZoneID: "zone1",
			},
			{
				FQDN:   "keeper3",
				ZoneID: "zone1",
			},
			{
				FQDN:   "new_keeper2",
				ZoneID: "zone2",
			},
			{
				FQDN:   "new_keeper3",
				ZoneID: "zone3",
			},
		}
		result, oldKeepers, newKeepers := updateKeeperHosts(keeperFQDNs, allHosts)
		require.True(t, result)
		require.ElementsMatch(t, oldKeepers, []hosts.Host{
			{
				FQDN:   "keeper1",
				ZoneID: "zone1",
			},
		})

		require.ElementsMatch(t, newKeepers, []hosts.Host{
			{
				FQDN:   "new_keeper2",
				ZoneID: "zone2",
			},
			{
				FQDN:   "new_keeper3",
				ZoneID: "zone3",
			},
		})
	})

}

func TestClickHouse_ValidateKeeperRemoval(t *testing.T) {
	t.Run("Remove 2 keepers", func(t *testing.T) {
		resourceChanges := clickHouseResourceChanges{
			shardsToDelete: []clusters.Shard{
				{ShardID: "shard1"},
			},
		}
		keeperFQNDs := []string{"host1", "host2", "host3"}
		shardHostMap := map[string][]hosts.HostExtended{
			"shard1": {
				{
					Host: hosts.Host{
						FQDN: "host1",
					},
				},
				{
					Host: hosts.Host{
						FQDN: "host2",
					},
				},
			},
			"shard2": {
				{
					Host: hosts.Host{
						FQDN: "host3",
					},
				},
				{
					Host: hosts.Host{
						FQDN: "host4",
					},
				},
			},
		}
		err := validateKeeperRemoval(resourceChanges, shardHostMap, keeperFQNDs)
		require.NoError(t, err)
	})

	t.Run("Remove all keepers", func(t *testing.T) {

		resourceChanges := clickHouseResourceChanges{
			shardsToDelete: []clusters.Shard{
				{ShardID: "shard1"},
			},
			hostsToDelete: []string{"host3"},
		}
		keeperFQNDs := []string{"host1", "host2", "host3"}
		shardHostMap := map[string][]hosts.HostExtended{
			"shard1": {
				{
					Host: hosts.Host{
						FQDN: "host1",
					},
				},
				{
					Host: hosts.Host{
						FQDN: "host2",
					},
				},
			},
			"shard2": {
				{
					Host: hosts.Host{
						FQDN: "host3",
					},
				},
				{
					Host: hosts.Host{
						FQDN: "host4",
					},
				},
			},
		}
		err := validateKeeperRemoval(resourceChanges, shardHostMap, keeperFQNDs)
		require.Error(t, err)
	})
}
