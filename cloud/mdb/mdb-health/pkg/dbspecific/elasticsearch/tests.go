package elasticsearch

import (
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/dbspecific/testhelpers"
)

// GetTestServices return tests cases for services
func GetTestServices() []testhelpers.TestService {
	return []testhelpers.TestService{
		{
			Input: testhelpers.RoleServiceHealth{
				RoleData: testhelpers.ServiceHealth{
					serviceES: types.ServiceStatusAlive,
				},
				RoleMaster: testhelpers.ServiceHealth{
					serviceES: types.ServiceStatusAlive,
				}},
			Expected: types.ClusterStatusAlive,
		},
		{
			Input: testhelpers.RoleServiceHealth{
				RoleData: testhelpers.ServiceHealth{
					serviceES: types.ServiceStatusAlive,
				},
				RoleMaster: testhelpers.ServiceHealth{
					serviceES: types.ServiceStatusDead,
				}},
			Expected: types.ClusterStatusDegraded,
		},
		{
			Input: testhelpers.RoleServiceHealth{
				RoleData: testhelpers.ServiceHealth{
					serviceES: types.ServiceStatusAlive,
				},
				RoleMaster: testhelpers.ServiceHealth{
					serviceES: types.ServiceStatusUnknown,
				}},
			Expected: types.ClusterStatusDegraded,
		},
		{
			Input: testhelpers.RoleServiceHealth{
				RoleData: testhelpers.ServiceHealth{
					serviceES: types.ServiceStatusDead,
				},
				RoleMaster: testhelpers.ServiceHealth{
					serviceES: types.ServiceStatusAlive,
				}},
			Expected: types.ClusterStatusDead,
		},
		{
			Input: testhelpers.RoleServiceHealth{
				RoleData: testhelpers.ServiceHealth{
					serviceES: types.ServiceStatusUnknown,
				},
				RoleMaster: testhelpers.ServiceHealth{
					serviceES: types.ServiceStatusAlive,
				}},
			Expected: types.ClusterStatusUnknown,
		},
		{
			Input: testhelpers.RoleServiceHealth{
				RoleData: testhelpers.ServiceHealth{
					serviceES: types.ServiceStatusDead,
				},
				RoleMaster: testhelpers.ServiceHealth{
					serviceES: types.ServiceStatusDead,
				}},
			Expected: types.ClusterStatusDead,
		},
		{
			Input: testhelpers.RoleServiceHealth{
				RoleData: testhelpers.ServiceHealth{
					serviceES: types.ServiceStatusUnknown,
				},
				RoleMaster: testhelpers.ServiceHealth{
					serviceES: types.ServiceStatusDead,
				}},
			Expected: types.ClusterStatusUnknown,
		},
		{
			Input: testhelpers.RoleServiceHealth{
				RoleData: testhelpers.ServiceHealth{
					serviceES: types.ServiceStatusDead,
				},
				RoleMaster: testhelpers.ServiceHealth{
					serviceES: types.ServiceStatusUnknown,
				}},
			Expected: types.ClusterStatusDead,
		},
		{
			Input: testhelpers.RoleServiceHealth{
				RoleData: testhelpers.ServiceHealth{
					serviceES: types.ServiceStatusUnknown,
				},
				RoleMaster: testhelpers.ServiceHealth{
					serviceES: types.ServiceStatusUnknown,
				}},
			Expected: types.ClusterStatusUnknown,
		},
		{
			Input: testhelpers.RoleServiceHealth{
				RoleData: testhelpers.ServiceHealth{
					serviceES: types.ServiceStatusAlive,
				}},
			Expected: types.ClusterStatusAlive,
		},
		{
			Input: testhelpers.RoleServiceHealth{
				RoleData: testhelpers.ServiceHealth{
					serviceES: types.ServiceStatusDead,
				}},
			Expected: types.ClusterStatusDead,
		},
		{
			Input: testhelpers.RoleServiceHealth{
				RoleData: testhelpers.ServiceHealth{
					serviceES: types.ServiceStatusUnknown,
				}},
			Expected: types.ClusterStatusUnknown,
		},
	}
}
