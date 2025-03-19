package redis

import (
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/dbspecific/testhelpers"
)

// GetTestServices return tests cases for services
func GetTestServices() []testhelpers.TestService {
	return []testhelpers.TestService{
		{
			Input: testhelpers.RoleServiceHealth{RoleRedis: testhelpers.ServiceHealth{
				ServiceRedis:    types.ServiceStatusAlive,
				ServiceSentinel: types.ServiceStatusAlive,
			}},
			Expected:   types.ClusterStatusAlive,
			ExpectHost: types.HostStatusAlive,
		},
		{
			Input: testhelpers.RoleServiceHealth{RoleRedis: testhelpers.ServiceHealth{
				ServiceRedis:    types.ServiceStatusAlive,
				ServiceSentinel: types.ServiceStatusDead,
			}},
			Expected:   types.ClusterStatusDead,
			ExpectHost: types.HostStatusDead,
		},
		{
			Input: testhelpers.RoleServiceHealth{RoleRedis: testhelpers.ServiceHealth{
				ServiceRedis:    types.ServiceStatusAlive,
				ServiceSentinel: types.ServiceStatusUnknown,
			}},
			Expected:   types.ClusterStatusUnknown,
			ExpectHost: types.HostStatusUnknown,
		},
		{
			Input: testhelpers.RoleServiceHealth{RoleRedis: testhelpers.ServiceHealth{
				ServiceRedis: types.ServiceStatusAlive,
			}},
			Expected:   types.ClusterStatusUnknown,
			ExpectHost: types.HostStatusUnknown,
		},
		{
			Input: testhelpers.RoleServiceHealth{RoleRedis: testhelpers.ServiceHealth{
				ServiceRedis:    types.ServiceStatusDead,
				ServiceSentinel: types.ServiceStatusAlive,
			}},
			Expected:   types.ClusterStatusDead,
			ExpectHost: types.HostStatusDead,
		},
		{
			Input: testhelpers.RoleServiceHealth{RoleRedis: testhelpers.ServiceHealth{
				ServiceRedis:    types.ServiceStatusUnknown,
				ServiceSentinel: types.ServiceStatusAlive,
			}},
			Expected:   types.ClusterStatusUnknown,
			ExpectHost: types.HostStatusUnknown,
		},
		{
			Input: testhelpers.RoleServiceHealth{RoleRedis: testhelpers.ServiceHealth{
				ServiceRedis:    types.ServiceStatusDead,
				ServiceSentinel: types.ServiceStatusDead,
			}},
			Expected:   types.ClusterStatusDead,
			ExpectHost: types.HostStatusDead,
		},
		{
			Input: testhelpers.RoleServiceHealth{RoleRedis: testhelpers.ServiceHealth{
				ServiceRedis:    types.ServiceStatusUnknown,
				ServiceSentinel: types.ServiceStatusDead,
			}},
			Expected:   types.ClusterStatusDead,
			ExpectHost: types.HostStatusDead,
		},
		{
			Input: testhelpers.RoleServiceHealth{RoleRedis: testhelpers.ServiceHealth{
				ServiceRedis:    types.ServiceStatusDead,
				ServiceSentinel: types.ServiceStatusUnknown,
			}},
			Expected:   types.ClusterStatusDead,
			ExpectHost: types.HostStatusDead,
		},
		{
			Input: testhelpers.RoleServiceHealth{RoleRedis: testhelpers.ServiceHealth{
				ServiceRedis:    types.ServiceStatusUnknown,
				ServiceSentinel: types.ServiceStatusUnknown,
			}},
			Expected:   types.ClusterStatusUnknown,
			ExpectHost: types.HostStatusUnknown,
		},
		{
			Input: testhelpers.RoleServiceHealth{RoleRedis: testhelpers.ServiceHealth{
				ServiceRedis: types.ServiceStatusDead,
			}},
			Expected:   types.ClusterStatusDead,
			ExpectHost: types.HostStatusDead,
		},
		{
			Input: testhelpers.RoleServiceHealth{RoleRedis: testhelpers.ServiceHealth{
				ServiceRedis: types.ServiceStatusUnknown,
			}},
			Expected:   types.ClusterStatusUnknown,
			ExpectHost: types.HostStatusUnknown,
		},
		{
			Input: testhelpers.RoleServiceHealth{RoleRedis: testhelpers.ServiceHealth{
				ServiceSentinel: types.ServiceStatusAlive,
			}},
			Expected:   types.ClusterStatusUnknown,
			ExpectHost: types.HostStatusUnknown,
		},
		{
			Input: testhelpers.RoleServiceHealth{RoleRedis: testhelpers.ServiceHealth{
				ServiceSentinel: types.ServiceStatusDead,
			}},
			Expected:   types.ClusterStatusDead,
			ExpectHost: types.HostStatusDead,
		},
		{
			Input: testhelpers.RoleServiceHealth{RoleRedis: testhelpers.ServiceHealth{
				ServiceSentinel: types.ServiceStatusUnknown,
			}},
			Expected:   types.ClusterStatusUnknown,
			ExpectHost: types.HostStatusUnknown,
		},
		{
			Input: testhelpers.RoleServiceHealth{RoleRedis: testhelpers.ServiceHealth{
				ServiceRedisCluster: types.ServiceStatusAlive,
			}},
			Expected:   types.ClusterStatusAlive,
			ExpectHost: types.HostStatusAlive,
		},
		{
			Input: testhelpers.RoleServiceHealth{RoleRedis: testhelpers.ServiceHealth{
				ServiceRedisCluster: types.ServiceStatusDead,
			}},
			Expected:   types.ClusterStatusDead,
			ExpectHost: types.HostStatusDead,
		},
		{
			Input: testhelpers.RoleServiceHealth{RoleRedis: testhelpers.ServiceHealth{
				ServiceRedisCluster: types.ServiceStatusUnknown,
			}},
			Expected:   types.ClusterStatusUnknown,
			ExpectHost: types.HostStatusUnknown,
		},
	}
}
