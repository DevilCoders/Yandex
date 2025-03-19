package sqlserver

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

func TestSQLServerEvaluateClusterHealth(t *testing.T) {
	tests := GetTestServices()
	var e extractor
	for i, test := range tests {
		t.Run(fmt.Sprintf("%s status %d", test.Expected, i), func(t *testing.T) {
			cid, fqdns, roles, health := testhelpers.GenerateOneHostsForRoleStatus(RoleToService(), test.Input)
			clusterHealth, err := e.EvaluateClusterHealth(cid, fqdns, roles, health)
			require.NoError(t, err)
			require.Equal(t, cid, clusterHealth.Cid)
			require.Equal(t, test.Expected, clusterHealth.Status)
		})
	}
}

func TestEvaluateClusterHealthWithBrokenRoles(t *testing.T) {
	e := extractor{}
	clusterHealth, err := e.EvaluateClusterHealth(
		"test-cid",
		nil,
		dbspecific.HostsMap{},
		dbspecific.HealthMap{},
	)
	require.Error(t, err)
	require.Equal(t, types.ClusterStatusUnknown, clusterHealth.Status)
}

func TestEvaluateClusterHealthForMultiNodeCluster(t *testing.T) {
	aliveMaster := []types.ServiceHealth{
		types.NewServiceHealth(
			serviceSqlserver,
			time.Now(),
			types.ServiceStatusAlive,
			types.ServiceRoleMaster,
			types.ServiceReplicaTypeUnknown,
			"",
			0,
			nil),
	}
	aliveReplica := []types.ServiceHealth{
		types.NewServiceHealth(
			serviceSqlserver,
			time.Now(),
			types.ServiceStatusAlive,
			types.ServiceRoleReplica,
			types.ServiceReplicaTypeSync,
			"",
			0,
			nil),
	}
	dead := []types.ServiceHealth{types.NewDeadHealthForService(serviceSqlserver)}
	roles := dbspecific.HostsMap{roleSQLServer: {"m1", "m2", "m3"}}

	cases := []struct {
		name          string
		health        dbspecific.HealthMap
		clusterStatus types.ClusterStatus
	}{
		{
			"all alive",
			dbspecific.HealthMap{
				"m1": aliveMaster,
				"m2": aliveReplica,
				"m3": aliveReplica,
			},
			types.ClusterStatusAlive,
		},
		{
			"one replica is dead",
			dbspecific.HealthMap{
				"m1": aliveMaster,
				"m2": aliveReplica,
				"m3": dead,
			},
			types.ClusterStatusDegraded,
		},
		{
			"master is dead (actually no master)",
			dbspecific.HealthMap{
				"m1": dead,
				"m2": aliveReplica,
				"m3": aliveReplica,
			},
			types.ClusterStatusDegraded,
		},
		{
			"there are 2 masters",
			dbspecific.HealthMap{
				"m1": aliveMaster,
				"m2": aliveMaster,
				"m3": aliveReplica,
			},
			types.ClusterStatusAlive,
		},
		{
			"one node is unknown",
			dbspecific.HealthMap{
				"m1": aliveMaster,
				"m2": aliveReplica,
			},
			types.ClusterStatusDegraded,
		},
		{
			"all nodes are dead",
			dbspecific.HealthMap{
				"m1": dead,
				"m2": dead,
				"m3": dead,
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

func TestEvaluateClusterHealthForTwoNodeCluster(t *testing.T) {
	aliveMaster := []types.ServiceHealth{
		types.NewServiceHealth(
			serviceSqlserver,
			time.Now(),
			types.ServiceStatusAlive,
			types.ServiceRoleMaster,
			types.ServiceReplicaTypeUnknown,
			"",
			0,
			nil),
	}
	aliveReplica := []types.ServiceHealth{
		types.NewServiceHealth(
			serviceSqlserver,
			time.Now(),
			types.ServiceStatusAlive,
			types.ServiceRoleReplica,
			types.ServiceReplicaTypeSync,
			"",
			0,
			nil),
	}
	aliveWitness := []types.ServiceHealth{
		types.NewServiceHealth(
			serviceWitness,
			time.Now(),
			types.ServiceStatusAlive,
			types.ServiceRoleUnknown,
			types.ServiceReplicaTypeUnknown,
			"",
			0,
			nil),
	}
	dead := []types.ServiceHealth{types.NewDeadHealthForService(serviceSqlserver)}
	deadWitness := []types.ServiceHealth{types.NewDeadHealthForService(serviceWitness)}
	roles := dbspecific.HostsMap{roleSQLServer: {"m2", "m3"}, roleWitness: {"m1"}}

	cases := []struct {
		name          string
		health        dbspecific.HealthMap
		clusterStatus types.ClusterStatus
	}{
		{
			"all alive",
			dbspecific.HealthMap{
				"m1": aliveWitness,
				"m2": aliveMaster,
				"m3": aliveReplica,
			},
			types.ClusterStatusAlive,
		},
		{
			"one replica is dead",
			dbspecific.HealthMap{
				"m1": aliveWitness,
				"m2": aliveMaster,
				"m3": dead,
			},
			types.ClusterStatusDegraded,
		},
		{
			"master is dead (actually no master)",
			dbspecific.HealthMap{
				"m1": aliveWitness,
				"m2": dead,
				"m3": aliveReplica,
			},
			types.ClusterStatusDegraded,
		},
		{
			"there are 2 masters",
			dbspecific.HealthMap{
				"m1": aliveWitness,
				"m2": aliveMaster,
				"m3": aliveMaster,
			},
			types.ClusterStatusAlive,
		},
		{
			"one node is unknown",
			dbspecific.HealthMap{
				"m1": aliveWitness,
				"m2": aliveMaster,
			},
			types.ClusterStatusDegraded,
		},
		{
			"witness node is dead",
			dbspecific.HealthMap{
				"m1": deadWitness,
				"m2": aliveMaster,
				"m3": aliveReplica,
			},
			types.ClusterStatusDegraded,
		},
		{
			"all nodes are dead",
			dbspecific.HealthMap{
				"m1": deadWitness,
				"m2": dead,
				"m3": dead,
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
				Roles: []string{roleSQLServer},
			}
		}
		return metadb.Host{
			FQDN:  geo + fmt.Sprintf("hostInClusterNumber%v", ind) + dbyandexnet,
			Geo:   geo,
			Roles: []string{roleSQLServer, string(metadb.MysqlCascadeReplicaRole)},
		}
	}

	type caseType struct {
		name       string
		hosts      []metadb.Host
		slaCluster bool
		slaShards  map[string]bool
	}

	cases := make([]caseType, 0)

	testCaseHosts := []metadb.Host{
		getter("man", true),
		getter("iva", true),
		getter("myt", true),
	}

	cases = append(cases, caseType{
		name:       "3 HA hosts in 3 different AZ",
		hosts:      testCaseHosts,
		slaCluster: true,
	})

	testCaseHosts = []metadb.Host{
		getter("man", true),
		getter("iva", false),
		getter("myt", true),
	}

	cases = append(cases, caseType{
		name:       "2 HA hosts in 3 different AZ",
		hosts:      testCaseHosts,
		slaCluster: true,
	})

	testCaseHosts = []metadb.Host{
		getter("man", true),
		getter("iva", true),
	}

	cases = append(cases, caseType{
		name: "2 HA hosts in 2 different AZ",
		hosts: []metadb.Host{
			testCaseHosts[0],
			testCaseHosts[1],
		},
		slaCluster: true,
	})

	testCaseHosts = []metadb.Host{
		getter("man", true),
	}

	cases = append(cases, caseType{
		name: "1 HA hosts in 1 different AZ",
		hosts: []metadb.Host{
			testCaseHosts[0],
		},
		slaCluster: false,
	})

	testCaseHosts = []metadb.Host{
		getter("man", true),
		getter("man", true),
		getter("man", true),
	}

	cases = append(cases, caseType{
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

	cases = append(cases, caseType{
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

	cases = append(cases, caseType{
		name: "2 HA hosts in 1 different AZ",
		hosts: []metadb.Host{
			testCaseHosts[0],
			testCaseHosts[2],
		},
		slaCluster: false,
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
