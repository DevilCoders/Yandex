package dbspecific

import (
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types"
)

func TestCalculateBrokenInfo(t *testing.T) {
	type testCase struct {
		name        string
		roles       HostsMap
		shards      HostsMap
		modes       ModeMap
		hostsBroken int
		dbBroken    int
		dbTotal     int
		hostsTotal  int
		dbRead      int
		dbWrite     int
	}

	cases := make([]testCase, 0)

	dataShard1HostMan := metadb.Host{
		ShardID: optional.NewString("shard1"),
		FQDN:    "man-data-shard1.db.yandex.net",
		Geo:     "man",
		Roles:   []string{},
	}
	dataShard1HostIva := metadb.Host{
		ShardID: optional.NewString("shard1"),
		FQDN:    "iva-data-shard1.db.yandex.net",
		Geo:     "iva",
		Roles:   []string{},
	}

	dataShard1HostMyt := metadb.Host{
		ShardID: optional.NewString("shard1"),
		FQDN:    "myt-data-shard1.db.yandex.net",
		Geo:     "myt",
		Roles:   []string{},
	}

	dataShard2HostMan := metadb.Host{
		ShardID: optional.NewString("shard2"),
		FQDN:    "man-data-shard2.db.yandex.net",
		Geo:     "man",
		Roles:   []string{},
	}
	dataShard2HostIva := metadb.Host{
		ShardID: optional.NewString("shard2"),
		FQDN:    "iva-data-shard2.db.yandex.net",
		Geo:     "iva",
		Roles:   []string{},
	}

	dataShard2HostMyt := metadb.Host{
		ShardID: optional.NewString("shard2"),
		FQDN:    "myt-data-shard2.db.yandex.net",
		Geo:     "myt",
		Roles:   []string{},
	}

	role := "role-role-role"

	cases = append(cases, testCase{
		name: "test not sharded with no broken hosts",
		roles: HostsMap{
			role: []string{
				dataShard1HostMan.FQDN,
				dataShard1HostMyt.FQDN,
				dataShard1HostIva.FQDN,
				dataShard2HostMan.FQDN,
				dataShard2HostMyt.FQDN,
				dataShard2HostIva.FQDN,
			},
		},
		modes: ModeMap{
			dataShard1HostIva.FQDN: types.Mode{
				Write:           true,
				Read:            true,
				UserFaultBroken: false,
			},
			dataShard1HostMan.FQDN: types.Mode{
				Write:           false,
				Read:            true,
				UserFaultBroken: false,
			},
			dataShard2HostMyt.FQDN: types.Mode{
				Write:           false,
				Read:            true,
				UserFaultBroken: false,
			},
			dataShard2HostIva.FQDN: types.Mode{
				Write:           false,
				Read:            true,
				UserFaultBroken: false,
			},
			dataShard2HostMan.FQDN: types.Mode{
				Write:           true,
				Read:            true,
				UserFaultBroken: false,
			},
			dataShard1HostMyt.FQDN: types.Mode{
				Write:           false,
				Read:            true,
				UserFaultBroken: false,
			},
		},
		hostsBroken: 0,
		dbBroken:    0,
		dbTotal:     1,
		hostsTotal:  6,
		dbRead:      1,
		dbWrite:     1,
	})

	cases = append(cases, testCase{
		name: "test shared with no broken hosts",
		shards: HostsMap{
			"shard1": []string{
				dataShard1HostIva.FQDN,
				dataShard1HostMan.FQDN,
				dataShard1HostMyt.FQDN},
			"shard2": []string{
				dataShard2HostIva.FQDN,
				dataShard2HostMan.FQDN,
				dataShard2HostMyt.FQDN},
		},
		roles: HostsMap{
			role: []string{
				dataShard1HostMan.FQDN,
				dataShard1HostMyt.FQDN,
				dataShard1HostIva.FQDN,
				dataShard2HostMan.FQDN,
				dataShard2HostMyt.FQDN,
				dataShard2HostIva.FQDN,
			},
		},
		modes: ModeMap{
			dataShard1HostIva.FQDN: types.Mode{
				Write:           true,
				Read:            true,
				UserFaultBroken: false,
			},
			dataShard1HostMan.FQDN: types.Mode{
				Write:           false,
				Read:            true,
				UserFaultBroken: false,
			},
			dataShard1HostMyt.FQDN: types.Mode{
				Write:           false,
				Read:            true,
				UserFaultBroken: false,
			},
			dataShard2HostMyt.FQDN: types.Mode{
				Write:           false,
				Read:            true,
				UserFaultBroken: false,
			},
			dataShard2HostIva.FQDN: types.Mode{
				Write:           false,
				Read:            true,
				UserFaultBroken: false,
			},
			dataShard2HostMan.FQDN: types.Mode{
				Write:           true,
				Read:            true,
				UserFaultBroken: false,
			},
		},
		hostsBroken: 0,
		dbBroken:    0,
		dbTotal:     2,
		hostsTotal:  6,
		dbRead:      2,
		dbWrite:     2,
	})

	cases = append(cases, testCase{
		name: "test shared with one broken shard",
		shards: HostsMap{
			"shard1": []string{
				dataShard1HostIva.FQDN,
				dataShard1HostMan.FQDN,
				dataShard1HostMyt.FQDN},
			"shard2": []string{
				dataShard2HostIva.FQDN,
				dataShard2HostMan.FQDN,
				dataShard2HostMyt.FQDN},
		},
		roles: HostsMap{
			role: []string{
				dataShard1HostMan.FQDN,
				dataShard1HostMyt.FQDN,
				dataShard1HostIva.FQDN,
				dataShard2HostMan.FQDN,
				dataShard2HostMyt.FQDN,
				dataShard2HostIva.FQDN,
			},
		},
		modes: ModeMap{
			dataShard1HostIva.FQDN: types.Mode{
				Write:           false,
				Read:            true,
				UserFaultBroken: true,
			},
			dataShard1HostMan.FQDN: types.Mode{
				Write:           false,
				Read:            true,
				UserFaultBroken: true,
			},
			dataShard2HostMyt.FQDN: types.Mode{
				Write:           false,
				Read:            true,
				UserFaultBroken: false,
			},
			dataShard2HostIva.FQDN: types.Mode{
				Write:           false,
				Read:            true,
				UserFaultBroken: false,
			},
			dataShard2HostMan.FQDN: types.Mode{
				Write:           true,
				Read:            true,
				UserFaultBroken: false,
			},
			dataShard1HostMyt.FQDN: types.Mode{
				Write:           false,
				Read:            true,
				UserFaultBroken: false,
			},
		},
		hostsBroken: 2,
		dbBroken:    1,
		dbTotal:     2,
		hostsTotal:  6,
		dbRead:      1,
		dbWrite:     1,
	})

	cases = append(cases, testCase{
		name: "test shared with one broken hosts by user AND service",
		shards: HostsMap{
			"shard1": []string{
				dataShard1HostIva.FQDN,
				dataShard1HostMan.FQDN,
				dataShard1HostMyt.FQDN},
			"shard2": []string{
				dataShard2HostIva.FQDN,
				dataShard2HostMan.FQDN,
				dataShard2HostMyt.FQDN},
		},
		roles: HostsMap{
			role: []string{
				dataShard1HostMan.FQDN,
				dataShard1HostMyt.FQDN,
				dataShard1HostIva.FQDN,
				dataShard2HostMan.FQDN,
				dataShard2HostMyt.FQDN,
				dataShard2HostIva.FQDN,
			},
		},
		modes: ModeMap{
			dataShard1HostIva.FQDN: types.Mode{
				Write:           false,
				Read:            true,
				UserFaultBroken: true,
			},
			dataShard1HostMan.FQDN: types.Mode{
				Write:           false,
				Read:            true,
				UserFaultBroken: true,
			},
			dataShard1HostMyt.FQDN: types.Mode{
				Write:           false,
				Read:            false,
				UserFaultBroken: false,
			},
			dataShard2HostMyt.FQDN: types.Mode{
				Write:           false,
				Read:            true,
				UserFaultBroken: false,
			},
			dataShard2HostIva.FQDN: types.Mode{
				Write:           false,
				Read:            true,
				UserFaultBroken: false,
			},
			dataShard2HostMan.FQDN: types.Mode{
				Write:           false,
				Read:            true,
				UserFaultBroken: false,
			},
		},
		hostsBroken: 2,
		dbBroken:    0,
		dbTotal:     2,
		hostsTotal:  6,
		dbRead:      2,
		dbWrite:     0,
	})

	cases = append(cases, testCase{
		name: "test sharded with 2 broken shards",
		shards: HostsMap{
			"shard1": []string{
				dataShard1HostIva.FQDN,
				dataShard1HostMan.FQDN,
				dataShard1HostMyt.FQDN},
			"shard2": []string{
				dataShard2HostIva.FQDN,
				dataShard2HostMan.FQDN,
				dataShard2HostMyt.FQDN},
		},
		roles: HostsMap{
			role: []string{
				dataShard1HostMan.FQDN,
				dataShard1HostMyt.FQDN,
				dataShard1HostIva.FQDN,
				dataShard2HostMan.FQDN,
				dataShard2HostMyt.FQDN,
				dataShard2HostIva.FQDN,
			},
		},
		modes: ModeMap{
			dataShard1HostIva.FQDN: types.Mode{
				Write:           false,
				Read:            true,
				UserFaultBroken: true,
			},
			dataShard1HostMan.FQDN: types.Mode{
				Write:           false,
				Read:            true,
				UserFaultBroken: true,
			},
			dataShard1HostMyt.FQDN: types.Mode{
				Write:           false,
				Read:            true,
				UserFaultBroken: false,
			},
			dataShard2HostMyt.FQDN: types.Mode{
				Write:           false,
				Read:            true,
				UserFaultBroken: true,
			},
			dataShard2HostIva.FQDN: types.Mode{
				Write:           false,
				Read:            true,
				UserFaultBroken: true,
			},
			dataShard2HostMan.FQDN: types.Mode{
				Write:           false,
				Read:            true,
				UserFaultBroken: false,
			},
		},
		hostsBroken: 4,
		dbBroken:    2,
		dbTotal:     2,
		hostsTotal:  6,
		dbRead:      0,
		dbWrite:     0,
	})

	cases = append(cases, testCase{
		name: "test shared with 2 broken shards and no userfault",
		shards: HostsMap{
			"shard1": []string{
				dataShard1HostIva.FQDN,
				dataShard1HostMan.FQDN,
				dataShard1HostMyt.FQDN},
			"shard2": []string{
				dataShard2HostIva.FQDN,
				dataShard2HostMan.FQDN,
				dataShard2HostMyt.FQDN},
		},
		roles: HostsMap{
			role: []string{
				dataShard1HostMan.FQDN,
				dataShard1HostMyt.FQDN,
				dataShard1HostIva.FQDN,
				dataShard2HostMan.FQDN,
				dataShard2HostMyt.FQDN,
				dataShard2HostIva.FQDN,
			},
		},
		modes: ModeMap{
			dataShard1HostIva.FQDN: types.Mode{
				Write:           false,
				Read:            true,
				UserFaultBroken: false,
			},
			dataShard1HostMan.FQDN: types.Mode{
				Write:           false,
				Read:            true,
				UserFaultBroken: false,
			},
			dataShard1HostMyt.FQDN: types.Mode{
				Write:           false,
				Read:            true,
				UserFaultBroken: false,
			},
			dataShard2HostMyt.FQDN: types.Mode{
				Write:           false,
				Read:            true,
				UserFaultBroken: false,
			},
			dataShard2HostIva.FQDN: types.Mode{
				Write:           false,
				Read:            true,
				UserFaultBroken: false,
			},
			dataShard2HostMan.FQDN: types.Mode{
				Write:           false,
				Read:            true,
				UserFaultBroken: false,
			},
		},
		hostsBroken: 0,
		dbBroken:    0,
		dbTotal:     2,
		hostsTotal:  6,
		dbRead:      2,
		dbWrite:     0,
	})

	cases = append(cases, testCase{
		name: "alive but userbroken shard not considered as broken",
		shards: HostsMap{
			"shard1": []string{
				dataShard1HostIva.FQDN,
				dataShard1HostMan.FQDN,
				dataShard1HostMyt.FQDN},
			"shard2": []string{
				dataShard2HostIva.FQDN,
				dataShard2HostMan.FQDN,
				dataShard2HostMyt.FQDN},
		},
		roles: HostsMap{
			role: []string{
				dataShard1HostMan.FQDN,
				dataShard1HostMyt.FQDN,
				dataShard1HostIva.FQDN,
				dataShard2HostMan.FQDN,
				dataShard2HostMyt.FQDN,
				dataShard2HostIva.FQDN,
			},
		},
		modes: ModeMap{
			dataShard1HostIva.FQDN: types.Mode{
				Write:           true,
				Read:            true,
				UserFaultBroken: true,
			},
			dataShard1HostMan.FQDN: types.Mode{
				Write:           false,
				Read:            true,
				UserFaultBroken: true,
			},
			dataShard2HostMyt.FQDN: types.Mode{
				Write:           false,
				Read:            true,
				UserFaultBroken: false,
			},
			dataShard2HostIva.FQDN: types.Mode{
				Write:           false,
				Read:            true,
				UserFaultBroken: false,
			},
			dataShard2HostMan.FQDN: types.Mode{
				Write:           true,
				Read:            true,
				UserFaultBroken: false,
			},
			dataShard1HostMyt.FQDN: types.Mode{
				Write:           false,
				Read:            true,
				UserFaultBroken: false,
			},
		},
		hostsBroken: 2,
		dbBroken:    0,
		dbTotal:     2,
		hostsTotal:  6,
		dbWrite:     2,
		dbRead:      2,
	})

	cases = append(cases, testCase{
		name: "test with no shards",
		roles: HostsMap{
			role: []string{
				dataShard1HostMan.FQDN,
				dataShard1HostMyt.FQDN,
				dataShard1HostIva.FQDN,
				dataShard2HostMan.FQDN,
				dataShard2HostMyt.FQDN,
				dataShard2HostIva.FQDN,
			},
		},
		modes: ModeMap{
			dataShard1HostIva.FQDN: types.Mode{
				Write:           true,
				Read:            true,
				UserFaultBroken: true,
			},
			dataShard1HostMan.FQDN: types.Mode{
				Write:           false,
				Read:            true,
				UserFaultBroken: true,
			},
			dataShard2HostMyt.FQDN: types.Mode{
				Write:           false,
				Read:            true,
				UserFaultBroken: false,
			},
			dataShard2HostIva.FQDN: types.Mode{
				Write:           false,
				Read:            true,
				UserFaultBroken: false,
			},
			dataShard2HostMan.FQDN: types.Mode{
				Write:           true,
				Read:            true,
				UserFaultBroken: false,
			},
			dataShard1HostMyt.FQDN: types.Mode{
				Write:           false,
				Read:            true,
				UserFaultBroken: false,
			},
		},
		hostsBroken: 2,
		dbBroken:    0,
		dbTotal:     1,
		hostsTotal:  6,
		dbWrite:     1,
		dbRead:      1,
	})

	cases = append(cases, testCase{
		name: "test with no shards, broken",
		roles: HostsMap{
			role: []string{
				dataShard1HostMan.FQDN,
				dataShard1HostMyt.FQDN,
				dataShard1HostIva.FQDN,
				dataShard2HostMan.FQDN,
				dataShard2HostMyt.FQDN,
				dataShard2HostIva.FQDN,
			},
		},
		modes: ModeMap{
			dataShard1HostIva.FQDN: types.Mode{
				Write:           true,
				Read:            true,
				UserFaultBroken: true,
			},
			dataShard1HostMan.FQDN: types.Mode{
				Write:           false,
				Read:            true,
				UserFaultBroken: true,
			},
			dataShard2HostMyt.FQDN: types.Mode{
				Write:           false,
				Read:            true,
				UserFaultBroken: true,
			},
			dataShard2HostIva.FQDN: types.Mode{
				Write:           false,
				Read:            true,
				UserFaultBroken: true,
			},
			dataShard2HostMan.FQDN: types.Mode{
				Write:           true,
				Read:            true,
				UserFaultBroken: true,
			},
			dataShard1HostMyt.FQDN: types.Mode{
				Write:           false,
				Read:            true,
				UserFaultBroken: true,
			},
		},
		hostsBroken: 6,
		dbBroken:    1,
		dbTotal:     1,
		hostsTotal:  6,
		dbWrite:     0,
		dbRead:      0,
	})

	for _, tcase := range cases {
		t.Run(tcase.name, func(t *testing.T) {

			res := CalcDBInfoCustom(role, tcase.roles, tcase.shards, tcase.modes, DefaultLimitFunc, CalculateBrokenInfoCustom)

			require.Equal(t, tcase.hostsBroken, res.HostsBrokenByUser)
			require.Equal(t, tcase.dbBroken, res.DBBroken)
			require.Equal(t, tcase.dbTotal, res.DBTotal)
			require.Equal(t, tcase.hostsTotal, res.HostsTotal)
			require.Equal(t, tcase.dbRead, res.DBRead)
			require.Equal(t, tcase.dbWrite, res.DBWrite)
		})
	}
}
