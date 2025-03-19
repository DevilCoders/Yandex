package clickhouse

import (
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/dbspecific/testhelpers"
)

// GetTestServices return tests cases for services
func GetTestServices() []testhelpers.TestService {
	return []testhelpers.TestService{
		{
			Input: testhelpers.RoleServiceHealth{
				RoleCH: testhelpers.ServiceHealth{
					serviceCH: types.ServiceStatusAlive,
				},
				roleZK: testhelpers.ServiceHealth{
					serviceZK: types.ServiceStatusAlive,
				}},
			Expected: types.ClusterStatusAlive,
		},
		{
			Input: testhelpers.RoleServiceHealth{
				RoleCH: testhelpers.ServiceHealth{
					serviceCH: types.ServiceStatusAlive,
				},
				roleZK: testhelpers.ServiceHealth{
					serviceZK: types.ServiceStatusDead,
				}},
			Expected: types.ClusterStatusDegraded,
		},
		{
			Input: testhelpers.RoleServiceHealth{
				RoleCH: testhelpers.ServiceHealth{
					serviceCH: types.ServiceStatusAlive,
				},
				roleZK: testhelpers.ServiceHealth{
					serviceZK: types.ServiceStatusUnknown,
				}},
			Expected: types.ClusterStatusDegraded,
		},
		{
			Input: testhelpers.RoleServiceHealth{
				RoleCH: testhelpers.ServiceHealth{
					serviceCH: types.ServiceStatusDead,
				},
				roleZK: testhelpers.ServiceHealth{
					serviceZK: types.ServiceStatusAlive,
				}},
			Expected: types.ClusterStatusDead,
		},
		{
			Input: testhelpers.RoleServiceHealth{
				RoleCH: testhelpers.ServiceHealth{
					serviceCH: types.ServiceStatusUnknown,
				},
				roleZK: testhelpers.ServiceHealth{
					serviceZK: types.ServiceStatusAlive,
				}},
			Expected: types.ClusterStatusUnknown,
		},
		{
			Input: testhelpers.RoleServiceHealth{
				RoleCH: testhelpers.ServiceHealth{
					serviceCH: types.ServiceStatusDead,
				},
				roleZK: testhelpers.ServiceHealth{
					serviceZK: types.ServiceStatusDead,
				}},
			Expected: types.ClusterStatusDead,
		},
		{
			Input: testhelpers.RoleServiceHealth{
				RoleCH: testhelpers.ServiceHealth{
					serviceCH: types.ServiceStatusUnknown,
				},
				roleZK: testhelpers.ServiceHealth{
					serviceZK: types.ServiceStatusDead,
				}},
			Expected: types.ClusterStatusUnknown,
		},
		{
			Input: testhelpers.RoleServiceHealth{
				RoleCH: testhelpers.ServiceHealth{
					serviceCH: types.ServiceStatusDead,
				},
				roleZK: testhelpers.ServiceHealth{
					serviceZK: types.ServiceStatusUnknown,
				}},
			Expected: types.ClusterStatusDead,
		},
		{
			Input: testhelpers.RoleServiceHealth{
				RoleCH: testhelpers.ServiceHealth{
					serviceCH: types.ServiceStatusUnknown,
				},
				roleZK: testhelpers.ServiceHealth{
					serviceZK: types.ServiceStatusUnknown,
				}},
			Expected: types.ClusterStatusUnknown,
		},
		{
			Input: testhelpers.RoleServiceHealth{
				RoleCH: testhelpers.ServiceHealth{
					serviceCH: types.ServiceStatusAlive,
				}},
			Expected: types.ClusterStatusAlive,
		},
		{
			Input: testhelpers.RoleServiceHealth{
				RoleCH: testhelpers.ServiceHealth{
					serviceCH: types.ServiceStatusDead,
				}},
			Expected: types.ClusterStatusDead,
		},
		{
			Input: testhelpers.RoleServiceHealth{
				RoleCH: testhelpers.ServiceHealth{
					serviceCH: types.ServiceStatusUnknown,
				}},
			Expected: types.ClusterStatusUnknown,
		},
	}
}
