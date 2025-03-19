package kafka_test

import (
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/katan/internal/tags"
	"a.yandex-team.ru/cloud/mdb/katan/katan/internal/cluster/planner"
	"a.yandex-team.ru/cloud/mdb/katan/katan/internal/cluster/planner/kafka"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types"
)

func makeHost(serviceName, geo, subcid string) planner.Host {
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
			},
		},
	}
}

func TestPlanner(t *testing.T) {
	t.Run("empty cluster is not ok", func(t *testing.T) {
		ret, err := kafka.Planner(planner.Cluster{})
		require.Error(t, err)
		require.Nil(t, ret)
	})

	t.Run("cluster with one host", func(t *testing.T) {
		ret, err := kafka.Planner(
			planner.Cluster{
				Hosts: map[string]planner.Host{
					"broker": {
						Services: map[string]planner.Service{
							kafka.KafkaServiceName: {
								Role: types.ServiceRoleUnknown,
							},
							kafka.ZKServiceName: {
								Role: types.ServiceRoleUnknown,
							},
						},
						Tags: tags.HostTags{
							Geo: "vla",
							Meta: tags.HostMeta{
								SubClusterID: "subcluster-kafka",
							},
						},
					},
				},
			},
		)

		require.NoError(t, err)
		require.Equal(t, [][]string{{"broker"}}, ret)
	})

	t.Run("3 broker cross-dc cluster", func(t *testing.T) {
		ret, err := kafka.Planner(
			planner.Cluster{
				Hosts: map[string]planner.Host{
					"broker1": makeHost(kafka.KafkaServiceName, "vla", "subcluster-kafka"),
					"broker2": makeHost(kafka.KafkaServiceName, "sas", "subcluster-kafka"),
					"broker3": makeHost(kafka.KafkaServiceName, "myt", "subcluster-kafka"),
					"zk1":     makeHost(kafka.KafkaServiceName, "vla", "subcluster-zk"),
					"zk2":     makeHost(kafka.KafkaServiceName, "sas", "subcluster-zk"),
					"zk3":     makeHost(kafka.KafkaServiceName, "myt", "subcluster-zk"),
				},
			},
		)

		require.NoError(t, err)
		require.Equal(t, [][]string{{"broker3"}, {"broker2"}, {"broker1"}, {"zk3"}, {"zk2"}, {"zk1"}}, ret)
	})

	t.Run("3 broker one-dc cluster", func(t *testing.T) {
		ret, err := kafka.Planner(
			planner.Cluster{
				Hosts: map[string]planner.Host{
					"broker1": makeHost(kafka.KafkaServiceName, "vla", "subcluster-kafka"),
					"broker2": makeHost(kafka.KafkaServiceName, "vla", "subcluster-kafka"),
					"broker3": makeHost(kafka.KafkaServiceName, "vla", "subcluster-kafka"),
					"zk1":     makeHost(kafka.KafkaServiceName, "vla", "subcluster-zk"),
					"zk2":     makeHost(kafka.KafkaServiceName, "sas", "subcluster-zk"),
					"zk3":     makeHost(kafka.KafkaServiceName, "myt", "subcluster-zk"),
				},
			},
		)

		require.NoError(t, err)
		require.Equal(t, [][]string{{"broker1"}, {"broker2", "broker3"}, {"zk3"}, {"zk2"}, {"zk1"}}, ret)
	})

	t.Run("6 brokers two-dc cluster", func(t *testing.T) {
		ret, err := kafka.Planner(
			planner.Cluster{
				Hosts: map[string]planner.Host{
					"broker1-vla": makeHost(kafka.KafkaServiceName, "vla", "subcluster-kafka"),
					"broker2-vla": makeHost(kafka.KafkaServiceName, "vla", "subcluster-kafka"),
					"broker3-vla": makeHost(kafka.KafkaServiceName, "vla", "subcluster-kafka"),
					"broker1-sas": makeHost(kafka.KafkaServiceName, "sas", "subcluster-kafka"),
					"broker2-sas": makeHost(kafka.KafkaServiceName, "sas", "subcluster-kafka"),
					"broker3-sas": makeHost(kafka.KafkaServiceName, "sas", "subcluster-kafka"),
					"zk1":         makeHost(kafka.KafkaServiceName, "vla", "subcluster-zk"),
					"zk2":         makeHost(kafka.KafkaServiceName, "sas", "subcluster-zk"),
					"zk3":         makeHost(kafka.KafkaServiceName, "myt", "subcluster-zk"),
				},
			},
		)

		require.NoError(t, err)
		require.Equal(t, [][]string{{"broker1-sas"}, {"broker2-sas", "broker3-sas"}, {"broker1-vla"}, {"broker2-vla", "broker3-vla"}, {"zk3"}, {"zk2"}, {"zk1"}}, ret)
	})
}
