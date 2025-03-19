package redis

import (
	"fmt"
	"testing"
	"time"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/dbspecific"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/dbspecific/testhelpers"
)

func TestRedisEvaluateClusterHealth(t *testing.T) {
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

func TestEvaluateClusterHealthForMultiNodeRedis(t *testing.T) {
	aliveMaster := []types.ServiceHealth{
		types.NewServiceHealth(
			ServiceRedis,
			time.Now(),
			types.ServiceStatusAlive,
			types.ServiceRoleMaster,
			types.ServiceReplicaTypeUnknown,
			"",
			0,
			nil),
		types.NewAliveHealthForService(ServiceSentinel),
	}
	closedMaster := []types.ServiceHealth{
		types.NewServiceHealth(
			ServiceRedis,
			time.Now(),
			types.ServiceStatusAlive,
			types.ServiceRoleMaster,
			types.ServiceReplicaTypeUnknown,
			"",
			0,
			nil),
		types.NewDeadHealthForService(ServiceSentinel),
	}
	aliveReplica := []types.ServiceHealth{
		types.NewServiceHealth(
			ServiceRedis,
			time.Now(),
			types.ServiceStatusAlive,
			types.ServiceRoleReplica,
			types.ServiceReplicaTypeSync,
			"",
			0,
			nil),
		types.NewAliveHealthForService(ServiceSentinel),
	}
	closedReplica := []types.ServiceHealth{
		types.NewServiceHealth(
			ServiceRedis,
			time.Now(),
			types.ServiceStatusAlive,
			types.ServiceRoleReplica,
			types.ServiceReplicaTypeSync,
			"",
			0,
			nil),
		types.NewDeadHealthForService(ServiceSentinel),
	}
	dead := []types.ServiceHealth{
		types.NewDeadHealthForService(ServiceRedis),
		types.NewDeadHealthForService(ServiceSentinel),
	}

	roles := dbspecific.HostsMap{RoleRedis: {"r1", "r2", "r3"}}

	cases := []struct {
		name          string
		health        dbspecific.HealthMap
		clusterStatus types.ClusterStatus
	}{
		{
			"all alive",
			dbspecific.HealthMap{
				"r1": aliveMaster,
				"r2": aliveReplica,
				"r3": aliveReplica,
			},
			types.ClusterStatusAlive,
		},
		{
			"one replica is dead",
			dbspecific.HealthMap{
				"r1": aliveMaster,
				"r2": aliveReplica,
				"r3": dead,
			},
			types.ClusterStatusDegraded,
		},
		{
			"master is dead (actually no master)",
			dbspecific.HealthMap{
				"r1": dead,
				"r2": aliveReplica,
				"r3": aliveReplica,
			},
			types.ClusterStatusDegraded,
		},
		{
			"master is closed",
			dbspecific.HealthMap{
				"r1": closedMaster,
				"r2": aliveReplica,
				"r3": aliveReplica,
			},
			types.ClusterStatusDegraded,
		},
		{
			"all nodes are closed",
			dbspecific.HealthMap{
				"r1": closedMaster,
				"r2": closedReplica,
				"r3": closedReplica,
			},
			types.ClusterStatusDead,
		},
		{
			"there are 2 masters",
			dbspecific.HealthMap{
				"r1": aliveMaster,
				"r2": aliveMaster,
				"r3": aliveReplica,
			},
			types.ClusterStatusAlive,
		},
		{
			"one node is unknown",
			dbspecific.HealthMap{
				"r1": aliveMaster,
				"r2": aliveReplica,
			},
			types.ClusterStatusDegraded,
		},
		{
			"all nodes are dead",
			dbspecific.HealthMap{
				"r1": dead,
				"r2": dead,
				"r3": dead,
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

func TestEvaluateClusterHealthForRedisCluster(t *testing.T) {
	aliveMaster := []types.ServiceHealth{
		types.NewServiceHealth(
			ServiceRedisCluster,
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
			ServiceRedisCluster,
			time.Now(),
			types.ServiceStatusAlive,
			types.ServiceRoleReplica,
			types.ServiceReplicaTypeSync,
			"",
			0,
			nil),
	}
	dead := []types.ServiceHealth{
		types.NewDeadHealthForService(ServiceRedisCluster),
	}

	roles := dbspecific.HostsMap{RoleRedis: {"r1.1", "r1.2", "r2.1", "r2.2"}}

	cases := []struct {
		name          string
		health        dbspecific.HealthMap
		clusterStatus types.ClusterStatus
	}{
		{
			"all alive",
			dbspecific.HealthMap{
				"r1.1": aliveMaster,
				"r1.2": aliveReplica,
				"r2.1": aliveMaster,
				"r2.2": aliveReplica,
			},
			types.ClusterStatusAlive,
		},
		{
			"one replica is dead",
			dbspecific.HealthMap{
				"r1.1": aliveMaster,
				"r1.2": aliveReplica,
				"r2.1": aliveMaster,
				"r2.2": dead,
			},
			types.ClusterStatusDegraded,
		},
		{
			"master is dead (actually no master)",
			dbspecific.HealthMap{
				"r1.1": dead,
				"r1.2": aliveReplica,
				"r2.1": aliveMaster,
				"r2.2": aliveReplica,
			},
			types.ClusterStatusDegraded,
		},
		{
			"one node is unknown",
			dbspecific.HealthMap{
				"r1.1": aliveMaster,
				"r1.2": aliveReplica,
				"r2.1": aliveReplica,
			},
			types.ClusterStatusDegraded,
		},
		{
			"all nodes are dead",
			dbspecific.HealthMap{
				"r1.1": dead,
				"r1.2": dead,
				"r2.1": dead,
				"r2.2": dead,
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
