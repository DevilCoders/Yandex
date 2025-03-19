package healthdbspec_test

import (
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/healthiness/healthbased/healthdbspec"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/dbspecific/mongodb"
)

func TestMongodbRoleResolver(t *testing.T) {
	type tCs struct {
		name    string
		ni      types.HostNeighboursInfo
		okLetGo bool
	}

	testCases := []tCs{
		{
			name: "mongos: all alive",
			ni: types.HostNeighboursInfo{
				Env:            healthdbspec.ProdEnvName,
				Roles:          []string{mongodb.RoleMongos},
				HACluster:      true,
				HAShard:        true,
				SameRolesTotal: 2,
				SameRolesAlive: 2,
			},
			okLetGo: true,
		},
		{
			name: "mongos: 2 of 3 alive",
			ni: types.HostNeighboursInfo{
				Env:            healthdbspec.ProdEnvName,
				Roles:          []string{mongodb.RoleMongos},
				HACluster:      true,
				HAShard:        true,
				SameRolesTotal: 2,
				SameRolesAlive: 1,
			},
			okLetGo: false,
		},
		{
			name: "mongos: 2 of 2 alive",
			ni: types.HostNeighboursInfo{
				Env:            healthdbspec.ProdEnvName,
				Roles:          []string{mongodb.RoleMongos},
				HACluster:      true,
				HAShard:        true,
				SameRolesTotal: 1,
				SameRolesAlive: 1,
			},
			okLetGo: true,
		},
		{
			name: "mongos: 1 of 2 alive",
			ni: types.HostNeighboursInfo{
				Env:            healthdbspec.ProdEnvName,
				Roles:          []string{mongodb.RoleMongos},
				HACluster:      true,
				HAShard:        true,
				SameRolesTotal: 1,
				SameRolesAlive: 0,
			},
			okLetGo: false,
		},
		{
			name: "mongos: last leg none prod",
			ni: types.HostNeighboursInfo{
				Env:            "none prod",
				Roles:          []string{mongodb.RoleMongos},
				HACluster:      true,
				HAShard:        true,
				SameRolesTotal: 1,
				SameRolesAlive: 0,
			},
			okLetGo: true,
		},
	}

	rr := healthdbspec.NewRoleSpecificResolver()
	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			require.Equal(t, tc.okLetGo, rr.OkToLetGo(tc.ni))
		})
	}
}
