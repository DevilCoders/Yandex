package sqlserver

import (
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/dbspecific/testhelpers"
)

// RoleToService return role to service map
func RoleToService() testhelpers.RoleService {
	return testhelpers.RoleService{
		roleSQLServer: []string{serviceSqlserver},
		roleWitness:   []string{serviceWitness},
	}
}

// GetTestServices return tests cases for services
func GetTestServices() []testhelpers.TestRole {
	return []testhelpers.TestRole{
		{
			Input: testhelpers.RoleStatus{
				roleSQLServer: types.ClusterStatusAlive,
			},
			Expected: types.ClusterStatusAlive,
		},
		{
			Input: testhelpers.RoleStatus{
				roleSQLServer: types.ClusterStatusDead,
			},
			Expected: types.ClusterStatusDead,
		},
		{
			Input: testhelpers.RoleStatus{
				roleSQLServer: types.ClusterStatusDegraded,
			},
			Expected: types.ClusterStatusDegraded,
		},
		{
			Input: testhelpers.RoleStatus{
				roleSQLServer: types.ClusterStatusUnknown,
			},
			Expected: types.ClusterStatusUnknown,
		},
		{
			Input: testhelpers.RoleStatus{
				roleSQLServer: types.ClusterStatusAlive,
				roleWitness:   types.ClusterStatusAlive,
			},
			Expected: types.ClusterStatusAlive,
		},
		{
			Input: testhelpers.RoleStatus{
				roleSQLServer: types.ClusterStatusAlive,
				roleWitness:   types.ClusterStatusUnknown,
			},
			Expected: types.ClusterStatusDegraded,
		},
		{
			Input: testhelpers.RoleStatus{
				roleSQLServer: types.ClusterStatusUnknown,
				roleWitness:   types.ClusterStatusAlive,
			},
			Expected: types.ClusterStatusUnknown,
		},
	}
}
