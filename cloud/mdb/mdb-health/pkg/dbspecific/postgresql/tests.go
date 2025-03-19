package postgresql

import (
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/dbspecific/testhelpers"
)

// GetTestServices return tests cases for services
func GetTestServices() []testhelpers.TestService {
	return []testhelpers.TestService{
		{
			Input: testhelpers.RoleServiceHealth{RolePG: testhelpers.ServiceHealth{
				ServiceReplication: types.ServiceStatusAlive,
				ServicePGBouncer:   types.ServiceStatusAlive,
			}},
			Expected:   types.ClusterStatusAlive,
			ExpectHost: types.HostStatusAlive,
		},
		{
			Input: testhelpers.RoleServiceHealth{RolePG: testhelpers.ServiceHealth{
				ServiceReplication: types.ServiceStatusAlive,
				ServicePGBouncer:   types.ServiceStatusDead,
			}},
			Expected:   types.ClusterStatusDead,
			ExpectHost: types.HostStatusDead,
		},
		{
			Input: testhelpers.RoleServiceHealth{RolePG: testhelpers.ServiceHealth{
				ServiceReplication: types.ServiceStatusAlive,
				ServicePGBouncer:   types.ServiceStatusUnknown,
			}},
			Expected:   types.ClusterStatusUnknown,
			ExpectHost: types.HostStatusUnknown,
		},
		{
			Input: testhelpers.RoleServiceHealth{RolePG: testhelpers.ServiceHealth{
				ServiceReplication: types.ServiceStatusAlive,
			}},
			Expected:   types.ClusterStatusUnknown,
			ExpectHost: types.HostStatusUnknown,
		},
		{
			Input: testhelpers.RoleServiceHealth{RolePG: testhelpers.ServiceHealth{
				ServiceReplication: types.ServiceStatusDead,
				ServicePGBouncer:   types.ServiceStatusAlive,
			}},
			Expected:   types.ClusterStatusDead,
			ExpectHost: types.HostStatusDead,
		},
		{
			Input: testhelpers.RoleServiceHealth{RolePG: testhelpers.ServiceHealth{
				ServiceReplication: types.ServiceStatusUnknown,
				ServicePGBouncer:   types.ServiceStatusAlive,
			}},
			Expected:   types.ClusterStatusUnknown,
			ExpectHost: types.HostStatusUnknown,
		},
		{
			Input: testhelpers.RoleServiceHealth{RolePG: testhelpers.ServiceHealth{
				ServiceReplication: types.ServiceStatusDead,
				ServicePGBouncer:   types.ServiceStatusDead,
			}},
			Expected:   types.ClusterStatusDead,
			ExpectHost: types.HostStatusDead,
		},
		{
			Input: testhelpers.RoleServiceHealth{RolePG: testhelpers.ServiceHealth{
				ServiceReplication: types.ServiceStatusUnknown,
				ServicePGBouncer:   types.ServiceStatusDead,
			}},
			Expected:   types.ClusterStatusDead,
			ExpectHost: types.HostStatusDead,
		},
		{
			Input: testhelpers.RoleServiceHealth{RolePG: testhelpers.ServiceHealth{
				ServiceReplication: types.ServiceStatusDead,
				ServicePGBouncer:   types.ServiceStatusUnknown,
			}},
			Expected:   types.ClusterStatusDead,
			ExpectHost: types.HostStatusDead,
		},
		{
			Input: testhelpers.RoleServiceHealth{RolePG: testhelpers.ServiceHealth{
				ServiceReplication: types.ServiceStatusUnknown,
				ServicePGBouncer:   types.ServiceStatusUnknown,
			}},
			Expected:   types.ClusterStatusUnknown,
			ExpectHost: types.HostStatusUnknown,
		},
		{
			Input: testhelpers.RoleServiceHealth{RolePG: testhelpers.ServiceHealth{
				ServiceReplication: types.ServiceStatusDead,
			}},
			Expected:   types.ClusterStatusDead,
			ExpectHost: types.HostStatusDead,
		},
		{
			Input: testhelpers.RoleServiceHealth{RolePG: testhelpers.ServiceHealth{
				ServiceReplication: types.ServiceStatusUnknown,
			}},
			Expected:   types.ClusterStatusUnknown,
			ExpectHost: types.HostStatusUnknown,
		},
		{
			Input: testhelpers.RoleServiceHealth{RolePG: testhelpers.ServiceHealth{
				ServicePGBouncer: types.ServiceStatusAlive,
			}},
			Expected:   types.ClusterStatusUnknown,
			ExpectHost: types.HostStatusUnknown,
		},
		{
			Input: testhelpers.RoleServiceHealth{RolePG: testhelpers.ServiceHealth{
				ServicePGBouncer: types.ServiceStatusDead,
			}},
			Expected:   types.ClusterStatusDead,
			ExpectHost: types.HostStatusDead,
		},
		{
			Input: testhelpers.RoleServiceHealth{RolePG: testhelpers.ServiceHealth{
				ServicePGBouncer: types.ServiceStatusUnknown,
			}},
			Expected:   types.ClusterStatusUnknown,
			ExpectHost: types.HostStatusUnknown,
		},
	}
}
