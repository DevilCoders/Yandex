package clickhouse

import (
	"fmt"
	"testing"
	"time"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/datastore"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/dbspecific"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/dbspecific/testhelpers"
)

func TestClickhouseEvaluateClusterHealth(t *testing.T) {
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
			dbspecific.HostsMap{RoleCH: {"c1"}, "strangeRole": {"strangeHost"}},
			dbspecific.HealthMap{
				"c1":          {types.NewAliveHealthForService(RoleCH)},
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
	// 2 ClickHouse nodes and 3 ZK nodes
	roles := dbspecific.HostsMap{
		RoleCH: {"c1", "c2"}, roleZK: {"z1", "z2", "z3"},
	}
	aliveClickHouse := []types.ServiceHealth{types.NewAliveHealthForService(serviceCH)}
	deadClickHouse := []types.ServiceHealth{types.NewServiceHealth(
		serviceCH,
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
				"c1": aliveClickHouse,
				"c2": aliveClickHouse,
				"z1": aliveZookeeper,
				"z2": aliveZookeeper,
				"z3": aliveZookeeper,
			},
			types.ClusterStatusAlive,
		},
		{
			"one ClickHouse is dead",
			dbspecific.HealthMap{
				"c1": aliveClickHouse,
				"c2": deadClickHouse,
				"z1": aliveZookeeper,
				"z2": aliveZookeeper,
				"z3": aliveZookeeper,
			},
			types.ClusterStatusDegraded,
		},
		{
			"one ClickHouse din't send metrics",
			dbspecific.HealthMap{
				"c1": aliveClickHouse,
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
				"c1": aliveClickHouse,
				"c2": aliveClickHouse,
				"z1": aliveZookeeper,
				"z2": aliveZookeeper,
				"z3": deadZookeeper,
			},
			types.ClusterStatusDegraded,
		},
		{
			"one Zookeeper is unknown",
			dbspecific.HealthMap{
				"c1": aliveClickHouse,
				"c2": aliveClickHouse,
				"z1": aliveZookeeper,
				"z2": aliveZookeeper,
				// z3 unknown
			},
			types.ClusterStatusDegraded,
		},
		{
			"one Zookeeper is unknown",
			dbspecific.HealthMap{
				"c1": aliveClickHouse,
				"c2": aliveClickHouse,
				"z1": aliveZookeeper,
				"z2": aliveZookeeper,
				"z3": deadZookeeper,
			},
			types.ClusterStatusDegraded,
		},
		{
			"all ClickHouse nodes are dead",
			dbspecific.HealthMap{
				"c1": deadClickHouse,
				"c2": deadClickHouse,
				"z1": aliveZookeeper,
				"z2": aliveZookeeper,
				"z3": aliveZookeeper,
			},
			types.ClusterStatusDead,
		},
		{
			"all ZooKeeper nodes are dead",
			dbspecific.HealthMap{
				"c1": aliveClickHouse,
				"c2": aliveClickHouse,
				"z1": deadZookeeper,
				"z2": deadZookeeper,
				"z3": deadZookeeper,
			},
			types.ClusterStatusDegraded,
		},
		{
			"all nodes are dead",
			dbspecific.HealthMap{
				"c1": deadClickHouse,
				"c2": deadClickHouse,
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
	getter := func(geo string, role string, shard int, new bool) metadb.Host {
		dbyandexnet := ".db.yandex.net"
		ind++
		hours := 2
		if new {
			hours = 0
		}
		return metadb.Host{
			FQDN:      geo + fmt.Sprintf("hostInClusterNumber%v", ind) + dbyandexnet,
			Geo:       geo,
			Roles:     []string{role},
			ShardID:   optional.NewString(fmt.Sprintf("shard%d", shard)),
			CreatedAt: time.Now().Add(-time.Duration(hours) * time.Hour),
		}
	}

	cases := []struct {
		name       string
		hosts      []metadb.Host
		slaCluster bool
		slaShards  map[string]bool
	}{
		{
			name: "3 ZK hosts in 3 different AZ, 2 CH hosts in different AZ",
			hosts: []metadb.Host{
				getter("man", roleZK, 0, false),
				getter("iva", roleZK, 0, false),
				getter("myt", roleZK, 0, false),
				getter("iva", RoleCH, 1, false),
				getter("myt", RoleCH, 1, false),
			},
			slaCluster: true,
			slaShards: map[string]bool{
				"shard1": true,
			},
		},
		{
			name: "3 ZK hosts in 3 different AZ, 2 CH shards witn 2 hosts in different AZ",
			hosts: []metadb.Host{
				getter("man", roleZK, 0, false),
				getter("iva", roleZK, 0, false),
				getter("myt", roleZK, 0, false),
				getter("iva", RoleCH, 1, false),
				getter("myt", RoleCH, 1, false),
				getter("iva", RoleCH, 2, false),
				getter("man", RoleCH, 2, false),
				getter("vla", RoleCH, 2, false),
			},
			slaCluster: true,
			slaShards: map[string]bool{
				"shard1": true,
				"shard2": true,
			},
		},
		{
			name: "3 ZK hosts in 3 different AZ, 2 CH shards witn 2 hosts in different AZ, second shard is new",
			hosts: []metadb.Host{
				getter("man", roleZK, 0, false),
				getter("iva", roleZK, 0, false),
				getter("myt", roleZK, 0, false),
				getter("iva", RoleCH, 1, false),
				getter("myt", RoleCH, 1, false),
				getter("iva", RoleCH, 2, true),
				getter("man", RoleCH, 2, true),
				getter("vla", RoleCH, 2, true),
			},
			slaCluster: true,
			slaShards: map[string]bool{
				"shard1": true,
				"shard2": false,
			},
		},
		{
			name: "3 ZK hosts in 3 different AZ, 2 CH shards witn 2 hosts in different AZ, one host in second shard is old",
			hosts: []metadb.Host{
				getter("man", roleZK, 0, false),
				getter("iva", roleZK, 0, false),
				getter("myt", roleZK, 0, false),
				getter("iva", RoleCH, 1, false),
				getter("myt", RoleCH, 1, false),
				getter("iva", RoleCH, 2, true),
				getter("man", RoleCH, 2, true),
				getter("vla", RoleCH, 2, false),
			},
			slaCluster: true,
			slaShards: map[string]bool{
				"shard1": true,
				"shard2": false,
			},
		},
		{
			name: "3 ZK hosts in 3 different AZ, 2 CH shards witn 2 hosts in different AZ, two hosts in second shard is old",
			hosts: []metadb.Host{
				getter("man", roleZK, 0, false),
				getter("iva", roleZK, 0, false),
				getter("myt", roleZK, 0, false),
				getter("iva", RoleCH, 1, false),
				getter("myt", RoleCH, 1, false),
				getter("iva", RoleCH, 2, true),
				getter("man", RoleCH, 2, false),
				getter("vla", RoleCH, 2, false),
			},
			slaCluster: true,
			slaShards: map[string]bool{
				"shard1": true,
				"shard2": true,
			},
		},
		{
			name: "3 ZK hosts in 3 different AZ, 2 CH shards, two hosts in second shard is old but in same AZ",
			hosts: []metadb.Host{
				getter("man", roleZK, 0, false),
				getter("iva", roleZK, 0, false),
				getter("myt", roleZK, 0, false),
				getter("iva", RoleCH, 1, false),
				getter("myt", RoleCH, 1, false),
				getter("iva", RoleCH, 2, true),
				getter("man", RoleCH, 2, false),
				getter("man", RoleCH, 2, false),
			},
			slaCluster: true,
			slaShards: map[string]bool{
				"shard1": true,
				"shard2": false,
			},
		},
		{
			name: "3 ZK hosts in 3 different AZ, 2 CH shards witn 2 hosts in same AZ",
			hosts: []metadb.Host{
				getter("man", roleZK, 0, false),
				getter("iva", roleZK, 0, false),
				getter("myt", roleZK, 0, false),
				getter("iva", RoleCH, 1, false),
				getter("myt", RoleCH, 1, false),
				getter("iva", RoleCH, 2, false),
				getter("iva", RoleCH, 2, false),
			},
			slaCluster: false,
			slaShards: map[string]bool{
				"shard1": true,
				"shard2": false,
			},
		},
		{
			name: "3 ZK hosts in 2 different AZ, 2 CH hosts in different AZ",
			hosts: []metadb.Host{
				getter("man", roleZK, 0, false),
				getter("iva", roleZK, 0, false),
				getter("man", roleZK, 0, false),
				getter("iva", RoleCH, 1, false),
				getter("myt", RoleCH, 1, false),
			},
			slaCluster: false,
			slaShards: map[string]bool{
				"shard1": false,
			},
		},
		{
			name: "1 ZK host, 2 CH hosts in different AZ",
			hosts: []metadb.Host{
				getter("man", roleZK, 0, false),
				getter("iva", RoleCH, 1, false),
				getter("myt", RoleCH, 1, false),
			},
			slaCluster: false,
			slaShards: map[string]bool{
				"shard1": false,
			},
		},
		{
			name: "2 ZK hosts in 2 different AZ, 2 CH hosts in different AZ",
			hosts: []metadb.Host{
				getter("man", roleZK, 0, false),
				getter("iva", roleZK, 0, false),
				getter("iva", RoleCH, 1, false),
				getter("myt", RoleCH, 1, false),
			},
			slaCluster: false,
			slaShards: map[string]bool{
				"shard1": false,
			},
		},
		{
			name: "4 ZK hosts in 4 different AZ, 2 CH hosts in different AZ",
			hosts: []metadb.Host{
				getter("man", roleZK, 0, false),
				getter("iva", roleZK, 0, false),
				getter("myt", roleZK, 0, false),
				getter("vla", roleZK, 0, false),
				getter("iva", RoleCH, 1, false),
				getter("myt", RoleCH, 1, false),
			},
			slaCluster: true,
			slaShards: map[string]bool{
				"shard1": true,
			},
		},
		{
			name: "4 ZK hosts in 3 different AZ, 2 CH hosts in different AZ",
			hosts: []metadb.Host{
				getter("man", roleZK, 0, false),
				getter("iva", roleZK, 0, false),
				getter("myt", roleZK, 0, false),
				getter("iva", roleZK, 0, false),
				getter("iva", RoleCH, 1, false),
				getter("myt", RoleCH, 1, false),
			},
			slaCluster: false,
			slaShards: map[string]bool{
				"shard1": false,
			},
		},
		{
			name: "5 ZK hosts in 5 different AZ, 2 CH hosts in different AZ",
			hosts: []metadb.Host{
				getter("man", roleZK, 0, false),
				getter("iva", roleZK, 0, false),
				getter("myt", roleZK, 0, false),
				getter("vla", roleZK, 0, false),
				getter("sas", roleZK, 0, false),
				getter("iva", RoleCH, 1, false),
				getter("myt", RoleCH, 1, false),
			},
			slaCluster: true,
			slaShards: map[string]bool{
				"shard1": true,
			},
		},
		{
			name: "5 ZK hosts in 3 different AZ without majority in one AZ, 2 CH hosts in different AZ",
			hosts: []metadb.Host{
				getter("man", roleZK, 0, false),
				getter("iva", roleZK, 0, false),
				getter("myt", roleZK, 0, false),
				getter("iva", roleZK, 0, false),
				getter("myt", roleZK, 0, false),
				getter("iva", RoleCH, 1, false),
				getter("myt", RoleCH, 1, false),
			},
			slaCluster: true,
			slaShards: map[string]bool{
				"shard1": true,
			},
		},
		{
			name: "5 ZK hosts in 3 different AZ with majority in one AZ, 2 CH hosts in different AZ",
			hosts: []metadb.Host{
				getter("man", roleZK, 0, false),
				getter("iva", roleZK, 0, false),
				getter("myt", roleZK, 0, false),
				getter("iva", roleZK, 0, false),
				getter("iva", roleZK, 0, false),
				getter("iva", RoleCH, 1, false),
				getter("myt", RoleCH, 1, false),
			},
			slaCluster: false,
			slaShards: map[string]bool{
				"shard1": false,
			},
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
			require.Equal(t, tt.slaShards, slaShards)
		})
	}
}

func TestCalculateUserBrokenInfo(t *testing.T) {
	type testCase struct {
		name            string
		status          string
		modes           dbspecific.ModeMap
		hostsUserBroken int
		dbUserBroken    int
		userBroken      map[string]struct{}
	}

	dataShard1HostMan := "man-data-shard1.db.yandex.net"
	dataShard1HostIva := "iva-data-shard1.db.yandex.net"
	dataShard1HostMyt := "myt-data-shard1.db.yandex.net"

	healthReadWrite := types.Mode{
		Write: true,
		Read:  true,
	}
	healthReadOnly := types.Mode{
		Read: true,
	}
	healthUserBroken := types.Mode{
		UserFaultBroken: true,
	}
	healthBroken := types.Mode{}

	cases := make([]testCase, 0)

	cases = append(cases, testCase{
		name: "test healthy shard",
		modes: dbspecific.ModeMap{
			dataShard1HostIva: healthReadWrite,
			dataShard1HostMan: healthReadWrite,
			dataShard1HostMyt: healthReadWrite,
		},
		hostsUserBroken: 0,
		dbUserBroken:    0,
		userBroken:      map[string]struct{}{},
	})

	cases = append(cases, testCase{
		name: "test broken shard",
		modes: dbspecific.ModeMap{
			dataShard1HostIva: healthBroken,
			dataShard1HostMan: healthBroken,
			dataShard1HostMyt: healthReadOnly,
		},
		hostsUserBroken: 0,
		dbUserBroken:    0,
		userBroken:      map[string]struct{}{},
	})

	cases = append(cases, testCase{
		name: "test user broken shard",
		modes: dbspecific.ModeMap{
			dataShard1HostIva: healthUserBroken,
			dataShard1HostMan: healthUserBroken,
			dataShard1HostMyt: healthReadOnly,
		},
		hostsUserBroken: 2,
		dbUserBroken:    1,
		userBroken:      map[string]struct{}{"shard1": {}},
	})

	cases = append(cases, testCase{
		name: "test broken shard, one user broken host",
		modes: dbspecific.ModeMap{
			dataShard1HostIva: healthUserBroken,
			dataShard1HostMan: healthBroken,
			dataShard1HostMyt: healthReadOnly,
		},
		hostsUserBroken: 1,
		dbUserBroken:    0,
		userBroken:      map[string]struct{}{},
	})

	cases = append(cases, testCase{
		name: "test broken shard, one user broken host, cluster modifying",
		modes: dbspecific.ModeMap{
			dataShard1HostIva: healthUserBroken,
			dataShard1HostMan: healthReadOnly,
			dataShard1HostMyt: healthReadOnly,
		},
		status:          "MODIFYING",
		hostsUserBroken: 1,
		dbUserBroken:    1,
		userBroken:      map[string]struct{}{"shard1": {}},
	})

	for _, tcase := range cases {
		t.Run(tcase.name, func(t *testing.T) {
			shardHosts := make([]string, 0, len(tcase.modes))
			for host := range tcase.modes {
				shardHosts = append(shardHosts, host)
			}

			info, mapBroken := calculateUserBrokenInfo(tcase.status)("", nil, map[string][]string{"shard1": shardHosts}, tcase.modes, dbspecific.DefaultLimitFunc)

			require.Equal(t, tcase.hostsUserBroken, info.HostsBrokenByUser)
			require.Equal(t, tcase.dbUserBroken, info.DBBroken)
			require.Equal(t, tcase.userBroken, mapBroken)
		})
	}
}
