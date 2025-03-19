package clickhouse_test

import (
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/katan/internal/tags"
	"a.yandex-team.ru/cloud/mdb/katan/katan/internal/cluster/planner"
	"a.yandex-team.ru/cloud/mdb/katan/katan/internal/cluster/planner/clickhouse"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types"
)

func makeHost(serviceName, geo, subcid string, shardID string) planner.Host {
	return planner.Host{
		Services: map[string]planner.Service{
			serviceName: {
				Role: types.ServiceRoleUnknown,
			},
		},
		Tags: tags.HostTags{
			Geo: geo,
			Meta: tags.HostMeta{
				SubClusterID: subcid,
				ShardID:      shardID,
			},
		},
	}
}

func makeClickhouseHost(geo, shardID string) planner.Host {
	return makeHost("clickhouse", geo, "subcid1", shardID)
}

func makeZookeeperHost(geo string) planner.Host {
	return makeHost("clickhouse", geo, "subcid2", "")
}

func TestPlanner(t *testing.T) {
	t.Run("empty cluster is not ok", func(t *testing.T) {
		ret, err := clickhouse.Planner(planner.Cluster{})
		require.Error(t, err)
		require.Nil(t, ret)
	})

	t.Run("cluster with one host", func(t *testing.T) {
		ret, err := clickhouse.Planner(
			planner.Cluster{
				Hosts: map[string]planner.Host{
					"sas-1": makeClickhouseHost("sas", "shard1"),
				},
			},
		)

		require.NoError(t, err)
		require.Equal(t, [][]string{{"sas-1"}}, ret)
	})

	t.Run("HA cluster with one shard", func(t *testing.T) {
		ret, err := clickhouse.Planner(
			planner.Cluster{
				Hosts: map[string]planner.Host{
					"sas-1": makeClickhouseHost("sas", "shard1"),
					"vla-1": makeClickhouseHost("vla", "shard1"),
					"myt-1": makeClickhouseHost("myt", "shard1"),
					"sas-2": makeZookeeperHost("sas"),
					"vla-2": makeZookeeperHost("vla"),
					"myt-2": makeZookeeperHost("myt"),
				},
			},
		)

		require.NoError(t, err)
		require.Equal(t, [][]string{
			{"myt-1"},
			{"sas-1"},
			{"vla-1"},
			{"myt-2"},
			{"sas-2"},
			{"vla-2"},
		}, ret)
	})

	t.Run("HA cluster with multiple shards", func(t *testing.T) {
		ret, err := clickhouse.Planner(
			planner.Cluster{
				Hosts: map[string]planner.Host{
					"sas-1": makeClickhouseHost("sas", "shard1"),
					"vla-1": makeClickhouseHost("vla", "shard1"),
					"myt-1": makeClickhouseHost("myt", "shard1"),
					"sas-2": makeClickhouseHost("sas", "shard2"),
					"vla-2": makeClickhouseHost("vla", "shard2"),
					"myt-2": makeClickhouseHost("myt", "shard2"),
					"sas-3": makeZookeeperHost("sas"),
					"vla-3": makeZookeeperHost("vla"),
					"myt-3": makeZookeeperHost("myt"),
				},
			},
		)

		require.NoError(t, err)
		require.Equal(t, [][]string{
			{"myt-1"},
			{"myt-2"},
			{"sas-1"},
			{"sas-2"},
			{"vla-1"},
			{"vla-2"},
			{"myt-3"},
			{"sas-3"},
			{"vla-3"},
		}, ret)
	})
}
