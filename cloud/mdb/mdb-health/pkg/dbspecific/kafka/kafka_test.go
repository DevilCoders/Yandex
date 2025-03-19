package kafka

import (
	"fmt"
	"testing"
	"time"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/datastore"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/dbspecific"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/dbspecific/testhelpers"
)

func TestKafkaEvaluateClusterHealth(t *testing.T) {
	tests := GetTestServices()
	var e extractor
	for i, test := range tests {
		t.Run(fmt.Sprintf("%s status %d", test.Expected, i), func(t *testing.T) {
			cid, fqdns, roles, health := testhelpers.GenerateOneHostWithAllServices(test.Input)
			clusterHealth, err := e.EvaluateClusterHealth(cid, fqdns, roles, health)
			require.NoError(t, err)
			require.Equal(t, cid, clusterHealth.Cid)
			require.Equal(t, test.Expected, clusterHealth.Status)
		})
	}
}

func TestEvaluateClusterHealthWithBrokenRoles(t *testing.T) {
	cases := []struct {
		name   string
		roles  dbspecific.HostsMap
		health dbspecific.HealthMap
	}{
		{
			"cluster without ClickHouse",
			dbspecific.HostsMap{roleZK: {"z1"}},
			dbspecific.HealthMap{
				"z1": {types.NewAliveHealthForService(serviceZK)},
			},
		},
		{
			"cluster with strange role",
			dbspecific.HostsMap{roleKafka: {"c1"}, "strangeRole": {"strangeHost"}},
			dbspecific.HealthMap{
				"c1":          {types.NewAliveHealthForService(roleKafka)},
				"strangeHost": nil,
			},
		},
	}

	for _, tt := range cases {
		t.Run(tt.name, func(t *testing.T) {
			e := extractor{}
			clusterHealth, err := e.EvaluateClusterHealth(
				"test-cid",
				nil,
				tt.roles,
				tt.health,
			)
			require.Error(t, err)
			require.Equal(t, types.ClusterStatusUnknown, clusterHealth.Status)
		})
	}
}

func TestEvaluateClusterHealthForMultiNodeCluster(t *testing.T) {
	// 2 Kafka nodes and 3 ZK nodes
	roles := dbspecific.HostsMap{
		roleKafka: {"c1", "c2"}, roleZK: {"z1", "z2", "z3"},
	}
	aliveBroker := []types.ServiceHealth{types.NewAliveHealthForService(serviceKafka)}
	deadBroker := []types.ServiceHealth{types.NewServiceHealth(
		serviceKafka,
		time.Now(),
		types.ServiceStatusDead,
		types.ServiceRoleUnknown,
		types.ServiceReplicaTypeUnknown,
		"",
		0,
		nil)}
	aliveZookeeper := []types.ServiceHealth{types.NewAliveHealthForService(serviceZK)}
	deadZookeeper := []types.ServiceHealth{types.NewServiceHealth(
		serviceZK,
		time.Now(),
		types.ServiceStatusDead,
		types.ServiceRoleUnknown,
		types.ServiceReplicaTypeUnknown,
		"",
		0,
		nil)}
	cases := []struct {
		name          string
		health        dbspecific.HealthMap
		clusterStatus types.ClusterStatus
	}{
		{
			"all alive",
			dbspecific.HealthMap{
				"c1": aliveBroker,
				"c2": aliveBroker,
				"z1": aliveZookeeper,
				"z2": aliveZookeeper,
				"z3": aliveZookeeper,
			},
			types.ClusterStatusAlive,
		},
		{
			"one Kafka is dead",
			dbspecific.HealthMap{
				"c1": aliveBroker,
				"c2": deadBroker,
				"z1": aliveZookeeper,
				"z2": aliveZookeeper,
				"z3": aliveZookeeper,
			},
			types.ClusterStatusDegraded,
		},
		{
			"one Kafka din't send metrics",
			dbspecific.HealthMap{
				"c1": aliveBroker,
				// c2 - no stats
				"z1": aliveZookeeper,
				"z2": aliveZookeeper,
				"z3": aliveZookeeper,
			},
			types.ClusterStatusDegraded,
		},
		{
			"one Zookeeper is dead",
			dbspecific.HealthMap{
				"c1": aliveBroker,
				"c2": aliveBroker,
				"z1": aliveZookeeper,
				"z2": aliveZookeeper,
				"z3": deadZookeeper,
			},
			types.ClusterStatusDegraded,
		},
		{
			"one Zookeeper is unknown",
			dbspecific.HealthMap{
				"c1": aliveBroker,
				"c2": aliveBroker,
				"z1": aliveZookeeper,
				"z2": aliveZookeeper,
				// z3 unknown
			},
			types.ClusterStatusDegraded,
		},
		{
			"one Zookeeper is unknown",
			dbspecific.HealthMap{
				"c1": aliveBroker,
				"c2": aliveBroker,
				"z1": aliveZookeeper,
				"z2": aliveZookeeper,
				"z3": deadZookeeper,
			},
			types.ClusterStatusDegraded,
		},
		{
			"all Kafka nodes are dead",
			dbspecific.HealthMap{
				"c1": deadBroker,
				"c2": deadBroker,
				"z1": aliveZookeeper,
				"z2": aliveZookeeper,
				"z3": aliveZookeeper,
			},
			types.ClusterStatusDead,
		},
		{
			"all ZooKeeper nodes are dead",
			dbspecific.HealthMap{
				"c1": aliveBroker,
				"c2": aliveBroker,
				"z1": deadZookeeper,
				"z2": deadZookeeper,
				"z3": deadZookeeper,
			},
			types.ClusterStatusDegraded,
		},
		{
			"all nodes are dead",
			dbspecific.HealthMap{
				"c1": deadBroker,
				"c2": deadBroker,
				"z1": deadZookeeper,
				"z2": deadZookeeper,
				"z3": deadZookeeper,
			},
			types.ClusterStatusDead,
		},
		{
			"all nodes are unknown",
			dbspecific.HealthMap{},
			types.ClusterStatusUnknown,
		},
	}

	for _, tt := range cases {
		t.Run(tt.name, func(t *testing.T) {
			e := extractor{}
			clusterHealth, err := e.EvaluateClusterHealth(
				"test-cid",
				nil,
				roles,
				tt.health,
			)
			require.NoError(t, err)
			require.Equal(t, tt.clusterStatus, clusterHealth.Status)
		})
	}
}

func TestHasSLAGuaranties(t *testing.T) {
	ind := 0
	getter := func(geo string, isHA bool) metadb.Host {
		dbyandexnet := ".db.yandex.net"
		// this is dirty hack to generate unique fdqn in tests (also, we can use random here)
		ind++
		if isHA {
			return metadb.Host{
				FQDN:  geo + fmt.Sprintf("hostInClusterNumber%v", ind) + dbyandexnet,
				Geo:   geo,
				Roles: []string{roleKafka},
			}
		}
		return metadb.Host{
			FQDN:  geo + fmt.Sprintf("hostInClusterNumber%v", ind) + dbyandexnet,
			Geo:   geo,
			Roles: []string{roleKafka, string(metadb.MysqlCascadeReplicaRole)},
		}
	}

	cases := make([]struct {
		name       string
		hosts      []metadb.Host
		slaCluster bool
		slaShards  map[string]bool
	}, 0)

	testCaseHosts := []metadb.Host{
		getter("man", true),
		getter("iva", true),
		getter("myt", true),
	}

	cases = append(cases, struct {
		name       string
		hosts      []metadb.Host
		slaCluster bool
		slaShards  map[string]bool
	}{
		name:       "3 HA hosts in 3 different AZ",
		hosts:      testCaseHosts,
		slaCluster: true,
		slaShards:  nil,
	})

	testCaseHosts = []metadb.Host{
		getter("man", true),
		getter("iva", false),
		getter("myt", true),
	}

	cases = append(cases, struct {
		name       string
		hosts      []metadb.Host
		slaCluster bool
		slaShards  map[string]bool
	}{
		name:       "2 HA hosts in 3 different AZ",
		hosts:      testCaseHosts,
		slaCluster: true,
		slaShards:  nil,
	})

	testCaseHosts = []metadb.Host{
		getter("man", true),
		getter("iva", true),
	}

	cases = append(cases, struct {
		name       string
		hosts      []metadb.Host
		slaCluster bool
		slaShards  map[string]bool
	}{
		name: "2 HA hosts in 2 different AZ",
		hosts: []metadb.Host{
			testCaseHosts[0],
			testCaseHosts[1],
		},
		slaCluster: true,
		slaShards:  nil,
	})

	testCaseHosts = []metadb.Host{
		getter("man", true),
	}

	cases = append(cases, struct {
		name       string
		hosts      []metadb.Host
		slaCluster bool
		slaShards  map[string]bool
	}{
		name: "1 HA hosts in 1 different AZ",
		hosts: []metadb.Host{
			testCaseHosts[0],
		},
		slaCluster: false,
		slaShards:  nil,
	})

	testCaseHosts = []metadb.Host{
		getter("man", true),
		getter("man", true),
		getter("man", true),
	}

	cases = append(cases, struct {
		name       string
		hosts      []metadb.Host
		slaCluster bool
		slaShards  map[string]bool
	}{
		name:       "3 HA hosts in 1 different AZ",
		hosts:      testCaseHosts,
		slaCluster: false,
		slaShards:  nil,
	})

	testCaseHosts = []metadb.Host{
		getter("man", true),
		getter("vla", true),
		getter("man", true),
	}

	cases = append(cases, struct {
		name       string
		hosts      []metadb.Host
		slaCluster bool
		slaShards  map[string]bool
	}{
		name:       "3 HA hosts in 2 different AZ",
		hosts:      testCaseHosts,
		slaCluster: true,
		slaShards:  nil,
	})

	testCaseHosts = []metadb.Host{
		getter("man", true),
		getter("vla", false),
		getter("man", true),
	}

	cases = append(cases, struct {
		name       string
		hosts      []metadb.Host
		slaCluster bool
		slaShards  map[string]bool
	}{
		name: "2 HA hosts in 1 different AZ",
		hosts: []metadb.Host{
			testCaseHosts[0],
			testCaseHosts[2],
		},
		slaCluster: false,
		slaShards:  nil,
	})
	for _, tt := range cases {
		t.Run(tt.name, func(t *testing.T) {

			e := extractor{}
			topology := datastore.ClusterTopology{
				CID: "cid-" + tt.name,
				Rev: 1,
				Cluster: metadb.Cluster{
					Name:        "cluster name: " + tt.name,
					Type:        metadb.MysqlCluster,
					Environment: "qa",
					Visible:     true,
					Status:      "RUNNING",
				},
				Hosts: tt.hosts,
			}

			slaCluster, slaShards := e.GetSLAGuaranties(topology)
			require.Equal(t, tt.slaCluster, slaCluster)
			require.Equal(t, tt.slaShards, slaShards)
		})
	}
}
