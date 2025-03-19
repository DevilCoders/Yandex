package mysql

import (
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/dbspecific/testhelpers"
)

// RoleToService return role to service map
func RoleToService() testhelpers.RoleService {
	return testhelpers.RoleService{
		RoleMY: []string{ServiceMY},
	}
}

// GetTestServices return tests cases for services
func GetTestServices() []testhelpers.TestRole {
	return []testhelpers.TestRole{
		{
			Input: testhelpers.RoleStatus{
				RoleMY: types.ClusterStatusAlive,
			},
			Expected: types.ClusterStatusAlive,
		},
		{
			Input: testhelpers.RoleStatus{
				RoleMY: types.ClusterStatusDead,
			},
			Expected: types.ClusterStatusDead,
		},
		{
			Input: testhelpers.RoleStatus{
				RoleMY: types.ClusterStatusDegraded,
			},
			Expected: types.ClusterStatusDegraded,
		},
		{
			Input: testhelpers.RoleStatus{
				RoleMY: types.ClusterStatusUnknown,
			},
			Expected: types.ClusterStatusUnknown,
		},
	}
}
