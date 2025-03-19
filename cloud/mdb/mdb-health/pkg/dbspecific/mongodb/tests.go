package mongodb

import (
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/dbspecific"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/dbspecific/testhelpers"
)

// RoleToService return role to service map
func RoleToService() testhelpers.RoleService {
	return testhelpers.RoleService{
		RoleMongod:     []string{ServiceMongod},
		RoleMongos:     []string{ServiceMongos},
		RoleMongocfg:   []string{ServiceMongocfg},
		RoleMongoinfra: []string{ServiceMongos, ServiceMongocfg},
	}
}

// GetTestServices return tests cases for services
func GetTestServices() []testhelpers.TestRole {
	return []testhelpers.TestRole{
		{
			Input: testhelpers.RoleStatus{
				RoleMongod: types.ClusterStatusAlive,
			},
			Expected: types.ClusterStatusAlive,
		},
		{
			Input: testhelpers.RoleStatus{
				RoleMongod:   types.ClusterStatusAlive,
				RoleMongos:   types.ClusterStatusAlive,
				RoleMongocfg: types.ClusterStatusAlive,
			},
			Expected: types.ClusterStatusAlive,
		},
		{
			Input: testhelpers.RoleStatus{
				RoleMongod:   types.ClusterStatusAlive,
				RoleMongos:   types.ClusterStatusAlive,
				RoleMongocfg: types.ClusterStatusDead,
			},
			Expected: types.ClusterStatusDead,
		},
		{
			Input: testhelpers.RoleStatus{
				RoleMongod:   types.ClusterStatusAlive,
				RoleMongos:   types.ClusterStatusAlive,
				RoleMongocfg: types.ClusterStatusUnknown,
			},
			Expected: types.ClusterStatusUnknown,
		},
		{
			Input: testhelpers.RoleStatus{
				RoleMongod:   types.ClusterStatusAlive,
				RoleMongos:   types.ClusterStatusAlive,
				RoleMongocfg: types.ClusterStatusDegraded,
			},
			Expected: types.ClusterStatusDegraded,
		},
		{
			Input: testhelpers.RoleStatus{
				RoleMongod:   types.ClusterStatusAlive,
				RoleMongos:   types.ClusterStatusDead,
				RoleMongocfg: types.ClusterStatusAlive,
			},
			Expected: types.ClusterStatusDead,
		},
		{
			Input: testhelpers.RoleStatus{
				RoleMongod:   types.ClusterStatusAlive,
				RoleMongos:   types.ClusterStatusUnknown,
				RoleMongocfg: types.ClusterStatusAlive,
			},
			Expected: types.ClusterStatusUnknown,
		},
		{
			Input: testhelpers.RoleStatus{
				RoleMongod:   types.ClusterStatusAlive,
				RoleMongos:   types.ClusterStatusDegraded,
				RoleMongocfg: types.ClusterStatusAlive,
			},
			Expected: types.ClusterStatusDegraded,
		},
		{
			Input: testhelpers.RoleStatus{
				RoleMongod:   types.ClusterStatusDead,
				RoleMongos:   types.ClusterStatusAlive,
				RoleMongocfg: types.ClusterStatusAlive,
			},
			Expected: types.ClusterStatusDead,
		},
		{
			Input: testhelpers.RoleStatus{
				RoleMongod:   types.ClusterStatusUnknown,
				RoleMongos:   types.ClusterStatusAlive,
				RoleMongocfg: types.ClusterStatusDead,
			},
			Expected: types.ClusterStatusDead,
		},
		{
			Input: testhelpers.RoleStatus{
				RoleMongod:   types.ClusterStatusDead,
				RoleMongos:   types.ClusterStatusDead,
				RoleMongocfg: types.ClusterStatusDead,
			},
			Expected: types.ClusterStatusDead,
		},
		{
			Input: testhelpers.RoleStatus{
				RoleMongod:   types.ClusterStatusUnknown,
				RoleMongos:   types.ClusterStatusUnknown,
				RoleMongocfg: types.ClusterStatusUnknown,
			},
			Expected: types.ClusterStatusUnknown,
		},
		{
			Input: testhelpers.RoleStatus{
				RoleMongod:     types.ClusterStatusAlive,
				RoleMongoinfra: types.ClusterStatusAlive,
			},
			Expected: types.ClusterStatusAlive,
		},
		{
			Input: testhelpers.RoleStatus{
				RoleMongod:     types.ClusterStatusAlive,
				RoleMongoinfra: types.ClusterStatusDead,
			},
			Expected: types.ClusterStatusDead,
		},
		{
			Input: testhelpers.RoleStatus{
				RoleMongod:     types.ClusterStatusDead,
				RoleMongoinfra: types.ClusterStatusAlive,
			},
			Expected: types.ClusterStatusDead,
		},
		{
			Input: testhelpers.RoleStatus{
				RoleMongod:     types.ClusterStatusAlive,
				RoleMongoinfra: types.ClusterStatusUnknown,
			},
			Expected: types.ClusterStatusUnknown,
		},
		{
			Input: testhelpers.RoleStatus{
				RoleMongod:     types.ClusterStatusAlive,
				RoleMongoinfra: types.ClusterStatusDegraded,
			},
			Expected: types.ClusterStatusDegraded,
		},
	}
}

// GetTestMode return tests cases for rw modes
func GetTestRWModes() testhelpers.TestGroupRWMode {
	return testhelpers.TestGroupRWMode{
		Roles: dbspecific.HostsMap{
			RoleMongod: {"s1d1", "s1d2", "s1d3", "s2d1", "s2d2", "s2d3", "s3d1", "s3d2", "s3d3"},
		},
		Shards: dbspecific.HostsMap{
			"s1": {"s1d1", "s1d2", "s1d3"},
			"s2": {"s2d1", "s2d2", "s2d3"},
			"s3": {"s3d1", "s3d2", "s3d3"},
		},
		Cases: []testhelpers.TestRWMode{
			{
				Name: "all read and write",
				HealthMode: dbspecific.ModeMap{
					"s1d1": types.Mode{Read: true, Write: true},
					"s1d2": types.Mode{Read: true, Write: true},
					"s1d3": types.Mode{Read: true, Write: true},

					"s2d1": types.Mode{Read: true, Write: true},
					"s2d2": types.Mode{Read: true, Write: true},
					"s2d3": types.Mode{Read: true, Write: true},

					"s3d1": types.Mode{Read: true, Write: true},
					"s3d2": types.Mode{Read: true, Write: true},
					"s3d3": types.Mode{Read: true, Write: true},
				},
				Result: types.DBRWInfo{
					HostsTotal: 9,
					HostsRead:  9,
					HostsWrite: 9,
					DBTotal:    3,
					DBRead:     3,
					DBWrite:    3,
				},
			},
			{
				Name: "all read and write, but some host in shards not work well",
				HealthMode: dbspecific.ModeMap{
					"s1d1": types.Mode{Read: true, Write: true},
					"s1d2": types.Mode{Read: false, Write: false},
					"s1d3": types.Mode{Read: true, Write: true},

					"s2d1": types.Mode{Read: true, Write: false},
					"s2d2": types.Mode{Read: true, Write: false},
					"s2d3": types.Mode{Read: true, Write: true},

					"s3d1": types.Mode{Read: false, Write: true},
					"s3d2": types.Mode{Read: true, Write: true},
					"s3d3": types.Mode{Read: true, Write: false},
				},
				Result: types.DBRWInfo{
					HostsTotal: 9,
					HostsRead:  7,
					HostsWrite: 5,
					DBTotal:    3,
					DBRead:     3,
					DBWrite:    3,
				},
			},
			{
				Name: "one shard RO",
				HealthMode: dbspecific.ModeMap{
					"s1d1": types.Mode{Read: true, Write: true},
					"s1d2": types.Mode{Read: false, Write: false},
					"s1d3": types.Mode{Read: true, Write: true},

					"s2d1": types.Mode{Read: true, Write: false},
					"s2d2": types.Mode{Read: false, Write: false},
					"s2d3": types.Mode{Read: true, Write: false},

					"s3d1": types.Mode{Read: true, Write: true},
					"s3d2": types.Mode{Read: true, Write: true},
					"s3d3": types.Mode{Read: true, Write: false},
				},
				Result: types.DBRWInfo{
					HostsTotal: 9,
					HostsRead:  7,
					HostsWrite: 4,
					DBTotal:    3,
					DBRead:     3,
					DBWrite:    2,
				},
			},
			{
				Name: "two shard RO",
				HealthMode: dbspecific.ModeMap{
					"s1d1": types.Mode{Read: true, Write: false},
					"s1d2": types.Mode{Read: false, Write: false},
					"s1d3": types.Mode{Read: true, Write: false},

					"s2d1": types.Mode{Read: true, Write: false},
					"s2d2": types.Mode{Read: false, Write: false},
					"s2d3": types.Mode{Read: true, Write: false},

					"s3d1": types.Mode{Read: true, Write: true},
					"s3d2": types.Mode{Read: true, Write: true},
					"s3d3": types.Mode{Read: true, Write: true},
				},
				Result: types.DBRWInfo{
					HostsTotal: 9,
					HostsRead:  7,
					HostsWrite: 3,
					DBTotal:    3,
					DBRead:     3,
					DBWrite:    1,
				},
			},
			{
				Name: "one RO and one DEAD",
				HealthMode: dbspecific.ModeMap{
					"s1d1": types.Mode{Read: false, Write: false},
					"s1d2": types.Mode{Read: false, Write: false},
					"s1d3": types.Mode{Read: false, Write: false},

					"s2d1": types.Mode{Read: true, Write: false},
					"s2d2": types.Mode{Read: true, Write: true},
					"s2d3": types.Mode{Read: true, Write: false},

					"s3d1": types.Mode{Read: true, Write: true},
					"s3d2": types.Mode{Read: true, Write: true},
					"s3d3": types.Mode{Read: false, Write: false},
				},
				Result: types.DBRWInfo{
					HostsTotal: 9,
					HostsRead:  5,
					HostsWrite: 3,
					DBTotal:    3,
					DBRead:     2,
					DBWrite:    2,
				},
			},
			{
				Name: "one RO and two DEAD",
				HealthMode: dbspecific.ModeMap{
					"s1d1": types.Mode{Read: false, Write: false},
					"s1d2": types.Mode{Read: false, Write: false},
					"s1d3": types.Mode{Read: false, Write: false},

					"s2d1": types.Mode{Read: true, Write: false},
					"s2d2": types.Mode{Read: false, Write: false},
					"s2d3": types.Mode{Read: true, Write: true},

					"s3d1": types.Mode{Read: false, Write: false},
					"s3d2": types.Mode{Read: false, Write: false},
					"s3d3": types.Mode{Read: false, Write: false},
				},
				Result: types.DBRWInfo{
					HostsTotal: 9,
					HostsRead:  2,
					HostsWrite: 1,
					DBTotal:    3,
					DBRead:     1,
					DBWrite:    1,
				},
			},
			{
				Name: "all not RO and  RW",
				HealthMode: dbspecific.ModeMap{
					"s1d1": types.Mode{Read: false, Write: false},
					"s1d2": types.Mode{Read: false, Write: false},
					"s1d3": types.Mode{Read: false, Write: false},

					"s2d1": types.Mode{Read: false, Write: false},
					"s2d2": types.Mode{Read: false, Write: false},
					"s2d3": types.Mode{Read: false, Write: false},

					"s3d1": types.Mode{Read: false, Write: false},
					"s3d2": types.Mode{Read: false, Write: false},
					"s3d3": types.Mode{Read: false, Write: false},
				},
				Result: types.DBRWInfo{
					HostsTotal: 9,
					HostsRead:  0,
					HostsWrite: 0,
					DBTotal:    3,
					DBRead:     0,
					DBWrite:    0,
				},
			},
		},
	}
}
