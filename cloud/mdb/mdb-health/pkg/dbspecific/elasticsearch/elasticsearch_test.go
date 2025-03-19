package elasticsearch

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

func TestElasticsearchEvaluateClusterHealth(t *testing.T) {
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
			"cluster without data nodes",
			dbspecific.HostsMap{RoleMaster: {"m1"}},
			dbspecific.HealthMap{
				"m1": {types.NewAliveHealthForService(serviceES)},
			},
		},
		{
			"cluster with strange role",
			dbspecific.HostsMap{RoleData: {"d1"}, "strangeRole": {"strangeHost"}},
			dbspecific.HealthMap{
				"d1":          {types.NewAliveHealthForService(RoleData)},
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
	// 2 data nodes and 3 master nodes
	roles := dbspecific.HostsMap{
		RoleData: {"d1", "d2"}, RoleMaster: {"m1", "m2", "m3"},
	}
	aliveES := []types.ServiceHealth{types.NewAliveHealthForService(serviceES)}
	deadES := []types.ServiceHealth{types.NewServiceHealth(
		serviceES,
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
				"d1": aliveES,
				"d2": aliveES,
				"m1": aliveES,
				"m2": aliveES,
				"m3": aliveES,
			},
			types.ClusterStatusAlive,
		},
		{
			"one data node is dead",
			dbspecific.HealthMap{
				"d1": aliveES,
				"d2": deadES,
				"m1": aliveES,
				"m2": aliveES,
				"m3": aliveES,
			},
			types.ClusterStatusDegraded,
		},
		{
			"one data node din't send metrics",
			dbspecific.HealthMap{
				"d1": aliveES,
				// d2 - no stats
				"m1": aliveES,
				"m2": aliveES,
				"m3": aliveES,
			},
			types.ClusterStatusDegraded,
		},
		{
			"one master is dead",
			dbspecific.HealthMap{
				"d1": aliveES,
				"d2": aliveES,
				"m1": aliveES,
				"m2": aliveES,
				"m3": deadES,
			},
			types.ClusterStatusDegraded,
		},
		{
			"one master is unknown",
			dbspecific.HealthMap{
				"d1": aliveES,
				"d2": aliveES,
				"m1": aliveES,
				"m2": aliveES,
				// m3 unknown
			},
			types.ClusterStatusDegraded,
		},
		{
			"all data nodes are dead",
			dbspecific.HealthMap{
				"d1": deadES,
				"d2": deadES,
				"m1": aliveES,
				"m2": aliveES,
				"m3": aliveES,
			},
			types.ClusterStatusDead,
		},
		{
			"all master nodes are dead",
			dbspecific.HealthMap{
				"d1": aliveES,
				"d2": aliveES,
				"m1": deadES,
				"m2": deadES,
				"m3": deadES,
			},
			types.ClusterStatusDegraded,
		},
		{
			"all nodes are dead",
			dbspecific.HealthMap{
				"d1": deadES,
				"d2": deadES,
				"m1": deadES,
				"m2": deadES,
				"m3": deadES,
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
	getter := func(geo string) metadb.Host {
		dbyandexnet := ".db.yandex.net"
		ind++
		return metadb.Host{
			FQDN:  geo + fmt.Sprintf("hostInClusterNumber%v", ind) + dbyandexnet,
			Geo:   geo,
			Roles: []string{RoleData},
		}
	}

	cases := []struct {
		name       string
		hosts      []metadb.Host
		slaCluster bool
	}{
		{
			name: "3 hosts in 3 different AZ",
			hosts: []metadb.Host{
				getter("man"),
				getter("iva"),
				getter("myt"),
			},
			slaCluster: true,
		},
		{
			name: "2 hosts in 2 different AZ",
			hosts: []metadb.Host{
				getter("man"),
				getter("iva"),
			},
			slaCluster: true,
		},
		{
			name: "1 hosts in 1 different AZ",
			hosts: []metadb.Host{
				getter("man"),
			},
			slaCluster: false,
		},
		{
			name: "3 hosts in 1 different AZ",
			hosts: []metadb.Host{
				getter("man"),
				getter("man"),
				getter("man"),
			},
			slaCluster: false,
		},
	}

	for _, tt := range cases {
		t.Run(tt.name, func(t *testing.T) {

			e := extractor{}
			topology := datastore.ClusterTopology{
				CID: "cid-" + tt.name,
				Rev: 1,
				Cluster: metadb.Cluster{
					Name:        "cluster name: " + tt.name,
					Type:        metadb.ElasticSearchCluster,
					Environment: "qa",
					Visible:     true,
					Status:      "RUNNING",
				},
				Hosts: tt.hosts,
			}

			slaCluster, slaShards := e.GetSLAGuaranties(topology)
			require.Equal(t, tt.slaCluster, slaCluster)
			require.Nil(t, slaShards)
		})
	}
}
