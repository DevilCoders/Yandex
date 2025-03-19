package kafka

import (
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/dbspecific/testhelpers"
)

// GetTestServices return tests cases for services
func GetTestServices() []testhelpers.TestService {
	return []testhelpers.TestService{
		{
			Input: testhelpers.RoleServiceHealth{
				roleKafka: testhelpers.ServiceHealth{
					serviceKafka: types.ServiceStatusAlive,
				},
				roleZK: testhelpers.ServiceHealth{
					serviceZK: types.ServiceStatusAlive,
				}},
			Expected: types.ClusterStatusAlive,
		},
		{
			Input: testhelpers.RoleServiceHealth{
				roleKafka: testhelpers.ServiceHealth{
					serviceKafka: types.ServiceStatusAlive,
				},
				roleZK: testhelpers.ServiceHealth{
					serviceZK: types.ServiceStatusDead,
				}},
			Expected: types.ClusterStatusDegraded,
		},
		{
			Input: testhelpers.RoleServiceHealth{
				roleKafka: testhelpers.ServiceHealth{
					serviceKafka: types.ServiceStatusAlive,
				},
				roleZK: testhelpers.ServiceHealth{
					serviceZK: types.ServiceStatusUnknown,
				}},
			Expected: types.ClusterStatusDegraded,
		},
		{
			Input: testhelpers.RoleServiceHealth{
				roleKafka: testhelpers.ServiceHealth{
					serviceKafka: types.ServiceStatusDead,
				},
				roleZK: testhelpers.ServiceHealth{
					serviceZK: types.ServiceStatusAlive,
				}},
			Expected: types.ClusterStatusDead,
		},
		{
			Input: testhelpers.RoleServiceHealth{
				roleKafka: testhelpers.ServiceHealth{
					serviceKafka: types.ServiceStatusUnknown,
				},
				roleZK: testhelpers.ServiceHealth{
					serviceZK: types.ServiceStatusAlive,
				}},
			Expected: types.ClusterStatusUnknown,
		},
		{
			Input: testhelpers.RoleServiceHealth{
				roleKafka: testhelpers.ServiceHealth{
					serviceKafka: types.ServiceStatusDead,
				},
				roleZK: testhelpers.ServiceHealth{
					serviceZK: types.ServiceStatusDead,
				}},
			Expected: types.ClusterStatusDead,
		},
		{
			Input: testhelpers.RoleServiceHealth{
				roleKafka: testhelpers.ServiceHealth{
					serviceKafka: types.ServiceStatusUnknown,
				},
				roleZK: testhelpers.ServiceHealth{
					serviceZK: types.ServiceStatusDead,
				}},
			Expected: types.ClusterStatusUnknown,
		},
		{
			Input: testhelpers.RoleServiceHealth{
				roleKafka: testhelpers.ServiceHealth{
					serviceKafka: types.ServiceStatusDead,
				},
				roleZK: testhelpers.ServiceHealth{
					serviceZK: types.ServiceStatusUnknown,
				}},
			Expected: types.ClusterStatusDead,
		},
		{
			Input: testhelpers.RoleServiceHealth{
				roleKafka: testhelpers.ServiceHealth{
					serviceKafka: types.ServiceStatusUnknown,
				},
				roleZK: testhelpers.ServiceHealth{
					serviceZK: types.ServiceStatusUnknown,
				}},
			Expected: types.ClusterStatusUnknown,
		},
		{
			Input: testhelpers.RoleServiceHealth{
				roleKafka: testhelpers.ServiceHealth{
					serviceKafka: types.ServiceStatusAlive,
				}},
			Expected: types.ClusterStatusAlive,
		},
		{
			Input: testhelpers.RoleServiceHealth{
				roleKafka: testhelpers.ServiceHealth{
					serviceKafka: types.ServiceStatusDead,
				}},
			Expected: types.ClusterStatusDead,
		},
		{
			Input: testhelpers.RoleServiceHealth{
				roleKafka: testhelpers.ServiceHealth{
					serviceKafka: types.ServiceStatusUnknown,
				}},
			Expected: types.ClusterStatusUnknown,
		},
	}
}
