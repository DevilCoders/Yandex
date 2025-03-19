package postgresql

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

func TestPostgreSQLEvaluateClusterHealth(t *testing.T) {
	tests := GetTestServices()
	var e extractor
	for i, test := range tests {
		t.Run(fmt.Sprintf("%s status %d", test.Expected, i), func(t *testing.T) {
			cid, fqdns, roles, health := testhelpers.GenerateOneHostWithAllServices(test.Input)
			clusterHealth, err := e.EvaluateClusterHealth(cid, fqdns, roles, health)
			require.NoError(t, err)
			require.Equal(t, cid, clusterHealth.Cid)
			require.Equal(t, test.Expected, clusterHealth.Status)
			for _, services := range health {
				status, _ := e.EvaluateHostHealth(services)
				require.Equal(t, test.ExpectHost, status)
			}
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
			ServiceReplication,
			time.Now(),
			types.ServiceStatusAlive,
			types.ServiceRoleMaster,
			types.ServiceReplicaTypeUnknown,
			"",
			0,
			nil),
		types.NewAliveHealthForService(ServicePGBouncer),
	}
	closedMaster := []types.ServiceHealth{
		types.NewServiceHealth(
			ServiceReplication,
			time.Now(),
			types.ServiceStatusAlive,
			types.ServiceRoleMaster,
			types.ServiceReplicaTypeUnknown,
			"",
			0,
			nil),
		types.NewDeadHealthForService(ServicePGBouncer),
	}
	aliveReplica := []types.ServiceHealth{
		types.NewServiceHealth(
			ServiceReplication,
			time.Now(),
			types.ServiceStatusAlive,
			types.ServiceRoleReplica,
			types.ServiceReplicaTypeSync,
			"",
			0,
			nil),
		types.NewAliveHealthForService(ServicePGBouncer),
	}
	closedReplica := []types.ServiceHealth{
		types.NewServiceHealth(
			ServiceReplication,
			time.Now(),
			types.ServiceStatusAlive,
			types.ServiceRoleReplica,
			types.ServiceReplicaTypeSync,
			"",
			0,
			nil),
		types.NewDeadHealthForService(ServicePGBouncer),
	}
	dead := []types.ServiceHealth{
		types.NewDeadHealthForService(ServiceReplication),
		types.NewDeadHealthForService(ServicePGBouncer),
	}

	roles := dbspecific.HostsMap{RolePG: {"p1", "p2", "p3"}}

	cases := []struct {
		name          string
		health        dbspecific.HealthMap
		clusterStatus types.ClusterStatus
	}{
		{
			"all alive",
			dbspecific.HealthMap{
				"p1": aliveMaster,
				"p2": aliveReplica,
				"p3": aliveReplica,
			},
			types.ClusterStatusAlive,
		},
		{
			"one replica is dead",
			dbspecific.HealthMap{
				"p1": aliveMaster,
				"p2": aliveReplica,
				"p3": dead,
			},
			types.ClusterStatusDegraded,
		},
		{
			"master is dead (actually no master)",
			dbspecific.HealthMap{
				"p1": dead,
				"p2": aliveReplica,
				"p3": aliveReplica,
			},
			types.ClusterStatusDegraded,
		},
		{
			"master is closed",
			dbspecific.HealthMap{
				"p1": closedMaster,
				"p2": aliveReplica,
				"p3": aliveReplica,
			},
			types.ClusterStatusDegraded,
		},
		{
			"all nodes are closed",
			dbspecific.HealthMap{
				"p1": closedMaster,
				"p2": closedReplica,
				"p3": closedReplica,
			},
			types.ClusterStatusDead,
		},
		{
			"there are 2 masters",
			dbspecific.HealthMap{
				"p1": aliveMaster,
				"p2": aliveMaster,
				"p3": aliveReplica,
			},
			types.ClusterStatusAlive,
		},
		{
			"one node is unknown",
			dbspecific.HealthMap{
				"p1": aliveMaster,
				"p2": aliveReplica,
			},
			types.ClusterStatusDegraded,
		},
		{
			"all nodes are dead",
			dbspecific.HealthMap{
				"p1": dead,
				"p2": dead,
				"p3": dead,
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
				Roles: []string{RolePG},
			}
		}
		return metadb.Host{
			FQDN:  geo + fmt.Sprintf("hostInClusterNumber%v", ind) + dbyandexnet,
			Geo:   geo,
			Roles: []string{RolePG, string(metadb.PostgresqlCascadeReplicaRole)},
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
					Type:        metadb.PostgresqlCluster,
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

func TestEvaluateGeoAtWarning(t *testing.T) {
	healthy := types.Mode{Write: true, Read: true}
	unhealthy := types.Mode{Write: false, Read: true}
	cases := []struct {
		name          string
		roles         dbspecific.HostsMap
		geos          dbspecific.HostsMap
		health        dbspecific.ModeMap
		geosAtWarning []string
	}{
		{
			name:          "healthy non-ha cluster",
			roles:         dbspecific.HostsMap{RolePG: {"f1-iva"}},
			geos:          dbspecific.HostsMap{"iva": {"f1-iva"}},
			health:        dbspecific.ModeMap{"f1-iva": healthy},
			geosAtWarning: nil,
		},
		{
			name:          "unhealthy non-ha cluster",
			roles:         dbspecific.HostsMap{RolePG: {"f1-iva"}},
			geos:          dbspecific.HostsMap{"iva": {"f1-iva"}},
			health:        dbspecific.ModeMap{"f1-iva": unhealthy},
			geosAtWarning: nil,
		},
		{
			name:          "unhealthy ha cluster",
			roles:         dbspecific.HostsMap{RolePG: {"f1-iva", "f1-sas"}},
			geos:          dbspecific.HostsMap{"iva": {"f1-iva"}, "sas": {"f1-sas"}},
			health:        dbspecific.ModeMap{"f1-iva": unhealthy, "f1-sas": healthy},
			geosAtWarning: []string{"sas"},
		},
		{
			name:          "unhealthy ha cluster with 3 node",
			roles:         dbspecific.HostsMap{RolePG: {"f1-iva", "f1-sas", "f1-myt"}},
			geos:          dbspecific.HostsMap{"iva": {"f1-iva"}, "sas": {"f1-sas"}, "myt": {"f1-myt"}},
			health:        dbspecific.ModeMap{"f1-iva": unhealthy, "f1-sas": healthy, "f1-myt": healthy},
			geosAtWarning: []string{"sas", "myt"},
		},
		{
			name:          "unhealthy ha cluster with 3 node with invalid az",
			roles:         dbspecific.HostsMap{RolePG: {"f1-iva", "f1-sas", "f2-sas"}},
			geos:          dbspecific.HostsMap{"iva": {"f1-iva"}, "sas": {"f1-sas", "f2-sas"}},
			health:        dbspecific.ModeMap{"f1-iva": healthy, "f1-sas": unhealthy, "f2-sas": unhealthy},
			geosAtWarning: []string{"iva"},
		},
		{
			name:          "ha cluster with unhealthy cascade",
			roles:         dbspecific.HostsMap{RolePG: {"f1-iva", "f1-sas"}, "cascade_replica": {"f2-sas"}},
			geos:          dbspecific.HostsMap{"iva": {"f1-iva"}, "sas": {"f1-sas", "f2-sas"}},
			health:        dbspecific.ModeMap{"f1-iva": healthy, "f1-sas": healthy, "f2-sas": unhealthy},
			geosAtWarning: nil,
		},
		{
			name:          "unhealthy ha cluster with healthy cascade",
			roles:         dbspecific.HostsMap{RolePG: {"f1-iva", "f1-sas"}, "cascade_replica": {"f2-sas"}},
			geos:          dbspecific.HostsMap{"iva": {"f1-iva"}, "sas": {"f1-sas", "f2-sas"}},
			health:        dbspecific.ModeMap{"f1-iva": healthy, "f1-sas": unhealthy, "f2-sas": healthy},
			geosAtWarning: []string{"iva"},
		},
		{
			name:          "unhealthy ha cluster without any healthy host",
			roles:         dbspecific.HostsMap{RolePG: {"f1-iva", "f1-sas", "f1-myt"}},
			geos:          dbspecific.HostsMap{"iva": {"f1-iva"}, "sas": {"f1-sas"}, "myt": {"f1-myt"}},
			health:        dbspecific.ModeMap{"f1-iva": unhealthy, "f1-sas": unhealthy, "f1-myt": unhealthy},
			geosAtWarning: nil, // What Is Dead May Never Die
		},
	}

	for _, tt := range cases {
		t.Run(tt.name, func(t *testing.T) {

			e := extractor{}

			geosAtWarning := e.EvaluateGeoAtWarning(tt.roles, nil, tt.geos, tt.health)
			require.ElementsMatch(t, tt.geosAtWarning, geosAtWarning)
		})
	}
}
