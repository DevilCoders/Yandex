package greenplum

import (
	"fmt"
	"testing"
	"time"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types"
	ct_testhelpers "a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types/testhelpers"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/dbspecific"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/dbspecific/testhelpers"
)

func TestGreenplumEvaluateClusterHealth(t *testing.T) {
	e := New()
	for i, test := range TestServices() {
		t.Run(fmt.Sprintf("%s status %d", test.Expected, i), func(t *testing.T) {
			cid, fqdns, roles, health := testhelpers.GenerateOneHostsForRoleStatus(RoleToService(), test.Input)
			clusterHealth, err := e.EvaluateClusterHealth(cid, fqdns, roles, health)
			require.NoError(t, err)
			require.Equal(t, cid, clusterHealth.Cid)
			require.Equal(t, test.Expected, clusterHealth.Status)
		})
	}
}

func TestGreenplumEvaluateClusterHealthAnyAlive(t *testing.T) {
	cid := "cid_test"
	fqdns := []string{"m_1", "t_1"}
	roles := dbspecific.HostsMap{RoleMaster: []string{"m_1"}, RoleSegments: []string{"t_1"}}
	health := dbspecific.HealthMap{
		"m_1": []types.ServiceHealth{
			createServiceHealth(ServiceMaster, types.ServiceStatusAlive),
			createServiceHealth(ServiceOdyssey, types.ServiceStatusAlive),
		},
		"t_1": []types.ServiceHealth{
			createServiceHealth(ServiceSegments, types.ServiceStatusDead),
			createServiceHealth(ServiceSegmentsAnyAlive, types.ServiceStatusAlive),
		}}

	e := New()
	clusterHealth, err := e.EvaluateClusterHealth(cid, fqdns, roles, health)
	require.NoError(t, err)
	require.Equal(t, cid, clusterHealth.Cid)
	require.Equal(t, types.ClusterStatusDegraded, clusterHealth.Status)
}
func TestGreenplumEvaluateClusterHealthOdyssey(t *testing.T) {
	cid := "cid_test"
	fqdns := []string{"m_1", "t_1"}
	roles := dbspecific.HostsMap{RoleMaster: []string{"m_1"}, RoleSegments: []string{"t_1"}}
	health := dbspecific.HealthMap{
		"m_1": []types.ServiceHealth{
			createServiceHealth(ServiceMaster, types.ServiceStatusAlive),
			createServiceHealth(ServiceOdyssey, types.ServiceStatusDead),
		},
		"t_1": []types.ServiceHealth{
			createServiceHealth(ServiceSegments, types.ServiceStatusAlive),
		}}

	e := New()
	clusterHealth, err := e.EvaluateClusterHealth(cid, fqdns, roles, health)
	require.NoError(t, err)
	require.Equal(t, cid, clusterHealth.Cid)
	require.Equal(t, types.ClusterStatusDegraded, clusterHealth.Status)
}

func TestEvaluateClusterHealthWithBrokenRoles(t *testing.T) {
	e := New()
	clusterHealth, err := e.EvaluateClusterHealth(
		"test-cid",
		nil,
		dbspecific.HostsMap{},
		dbspecific.HealthMap{},
	)
	require.Error(t, err)
	require.Equal(t, types.ClusterStatusUnknown, clusterHealth.Status)
}

func TestDBEvaluateDBReadWriteInfo(t *testing.T) {
	var e extractor
	tests := GetTestRWModes()
	for _, test := range tests.Cases {
		t.Run(test.Name, func(t *testing.T) {
			info, err := e.EvaluateDBInfo("test-cid", "RUNNING", tests.Roles, tests.Shards, test.HealthMode, nil)
			require.NoError(t, err)
			require.Equal(t, test.Result, info)
		})
	}
}

func TestGreenplumEvaluateHostHealth(t *testing.T) {
	e := New()

	for _, test := range testEvaluateHostHealth() {
		t.Run(test.name, func(t *testing.T) {
			status, _ := e.EvaluateHostHealth(test.services)
			if status != test.hostStatus {
				t.Errorf("Fail status %v expected %v", status, test.hostStatus)
			}
		})
	}
}

func testEvaluateHostHealth() []testHostHealth {
	return []testHostHealth{
		{
			name: "gp: master Alive",
			services: []types.ServiceHealth{
				createServiceHealth(ServiceMaster, types.ServiceStatusAlive),
				createServiceHealth(ServiceOdyssey, types.ServiceStatusAlive),
			},
			hostStatus: types.HostStatusAlive,
		},
		{
			name: "gp: master Degraded odyssey unknown",
			services: []types.ServiceHealth{
				createServiceHealth(ServiceMaster, types.ServiceStatusAlive),
			},
			hostStatus: types.HostStatusDegraded,
		},
		{
			name: "gp: master Degraded odyssey dead",
			services: []types.ServiceHealth{
				createServiceHealth(ServiceMaster, types.ServiceStatusAlive),
				createServiceHealth(ServiceOdyssey, types.ServiceStatusDead),
			},
			hostStatus: types.HostStatusDegraded,
		},
		{
			name: "gp: master Dead",
			services: []types.ServiceHealth{
				createServiceHealth(ServiceMaster, types.ServiceStatusDead),
				createServiceHealth(ServiceOdyssey, types.ServiceStatusAlive),
			},
			hostStatus: types.HostStatusDead,
		},
		{
			name: "gp: master Unknown",
			services: []types.ServiceHealth{
				createServiceHealth(ServiceMaster, types.ServiceStatusUnknown),
			},
			hostStatus: types.HostStatusUnknown,
		},

		{
			name: "gp: segment Alive",
			services: []types.ServiceHealth{
				createServiceHealth(ServiceSegments, types.ServiceStatusAlive),
			},
			hostStatus: types.HostStatusAlive,
		},
		{
			name: "gp: segment Alive and any dead",
			services: []types.ServiceHealth{
				createServiceHealth(ServiceSegments, types.ServiceStatusAlive),
				createServiceHealth(ServiceSegmentsAnyAlive, types.ServiceStatusDead),
			},
			hostStatus: types.HostStatusAlive,
		},
		{
			name: "gp: segment Degraded",
			services: []types.ServiceHealth{
				createServiceHealth(ServiceSegments, types.ServiceStatusDead),
				createServiceHealth(ServiceSegmentsAnyAlive, types.ServiceStatusAlive),
			},
			hostStatus: types.HostStatusDegraded,
		},
		{
			name: "gp: segment Degraded when segment status unkonown",
			services: []types.ServiceHealth{
				createServiceHealth(ServiceSegments, types.ServiceStatusUnknown),
				createServiceHealth(ServiceSegmentsAnyAlive, types.ServiceStatusAlive),
			},
			hostStatus: types.HostStatusDegraded,
		},
		{
			name: "gp: segment Dead",
			services: []types.ServiceHealth{
				createServiceHealth(ServiceSegments, types.ServiceStatusDead),
			},
			hostStatus: types.HostStatusDead,
		},
		{
			name: "gp: segment Unknown",
			services: []types.ServiceHealth{
				createServiceHealth(ServiceSegments, types.ServiceStatusUnknown),
			},
			hostStatus: types.HostStatusUnknown,
		},
	}
}

func createServiceHealth(name string, status types.ServiceStatus) types.ServiceHealth {
	metrics := ct_testhelpers.NewMetrics(0)
	return types.NewServiceHealth(name, time.Now(), status, types.ServiceRoleMaster, types.ServiceReplicaTypeUnknown, "", 0, metrics)
}
