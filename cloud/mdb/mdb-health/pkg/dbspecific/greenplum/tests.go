package greenplum

import (
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/dbspecific"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/dbspecific/testhelpers"
)

// RoleToService return role to service map
func RoleToService() testhelpers.RoleService {
	return testhelpers.RoleService{
		RoleMaster:   []string{ServiceOdyssey, ServiceMaster},
		RoleSegments: []string{ServiceSegments},
	}
}

// TestServices return tests cases for services
func TestServices() []testhelpers.TestRole {
	return []testhelpers.TestRole{
		{
			Input: testhelpers.RoleStatus{
				RoleMaster:   types.ClusterStatusAlive,
				RoleSegments: types.ClusterStatusAlive,
			},
			Expected: types.ClusterStatusAlive,
		},
		{
			Input: testhelpers.RoleStatus{
				RoleMaster:   types.ClusterStatusDead,
				RoleSegments: types.ClusterStatusAlive,
			},
			Expected: types.ClusterStatusDead,
		},
		{
			Input: testhelpers.RoleStatus{
				RoleMaster:   types.ClusterStatusDegraded,
				RoleSegments: types.ClusterStatusAlive,
			},
			Expected: types.ClusterStatusDegraded,
		},
		{
			Input: testhelpers.RoleStatus{
				RoleMaster:   types.ClusterStatusUnknown,
				RoleSegments: types.ClusterStatusAlive,
			},
			Expected: types.ClusterStatusUnknown,
		},
	}
}

// GetTestMode return tests cases for rw modes
func GetTestRWModes() testhelpers.TestGroupRWMode {
	return testhelpers.TestGroupRWMode{
		Roles: dbspecific.HostsMap{
			RoleMaster:   {"gplm-1", "gplm-2"},
			RoleSegments: {"gplm-3", "gplm-4", "gplm-5", "gplm-6"},
		},
		Cases: []testhelpers.TestRWMode{
			{
				Name: "all read and write",
				HealthMode: dbspecific.ModeMap{
					"gplm-1": types.Mode{Read: true, Write: true},  /* master */
					"gplm-2": types.Mode{Read: true, Write: false}, /* stanby */

					"gplm-3": types.Mode{Read: true, Write: true},
					"gplm-4": types.Mode{Read: true, Write: true},
					"gplm-5": types.Mode{Read: true, Write: true},
					"gplm-6": types.Mode{Read: true, Write: true},
				},
				Result: types.DBRWInfo{
					HostsTotal: 6,
					HostsRead:  6,
					HostsWrite: 5,
					DBTotal:    2,
					DBRead:     2,
					DBWrite:    2,
				},
			},
			{
				Name: "all read and write, but one segment is dead",
				HealthMode: dbspecific.ModeMap{
					"gplm-1": types.Mode{Read: true, Write: true},  /* master */
					"gplm-2": types.Mode{Read: true, Write: false}, /* stanby */

					"gplm-3": types.Mode{Read: true, Write: true},
					"gplm-4": types.Mode{Read: true, Write: true},
					"gplm-5": types.Mode{Read: false, Write: false},
					"gplm-6": types.Mode{Read: true, Write: true},
				},
				Result: types.DBRWInfo{
					HostsTotal: 6,
					HostsRead:  5,
					HostsWrite: 4,
					DBTotal:    2,
					DBRead:     1,
					DBWrite:    1,
				},
			},
			{
				Name: "dead master",
				HealthMode: dbspecific.ModeMap{
					"gplm-1": types.Mode{Read: false, Write: false}, /* master but dead */
					"gplm-2": types.Mode{Read: true, Write: false},  /* stanby */

					"gplm-3": types.Mode{Read: true, Write: true},
					"gplm-4": types.Mode{Read: true, Write: true},
					"gplm-5": types.Mode{Read: true, Write: true},
					"gplm-6": types.Mode{Read: true, Write: true},
				},
				Result: types.DBRWInfo{
					HostsTotal: 6,
					HostsRead:  5,
					HostsWrite: 4,
					DBTotal:    2,
					DBRead:     2,
					DBWrite:    1,
				},
			},
		},
	}
}

type testHostHealth struct {
	name       string
	services   []types.ServiceHealth
	hostStatus types.HostStatus
}
