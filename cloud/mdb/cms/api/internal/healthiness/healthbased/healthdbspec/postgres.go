package healthdbspec

import (
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/dbspecific/postgresql"
)

func pgLetGo(ni types.HostNeighboursInfo) bool {
	if configurationBasedCondition(ni) {
		return true
	}
	// https://st.yandex-team.ru/MDB-9193#5f197932e1a97d761b74f322
	return ni.SameRolesTotal == ni.SameRolesAlive
}

func registerPG() []RoleResolver {
	return []RoleResolver{
		{
			func(s string) bool { return s == postgresql.RolePG },
			pgLetGo,
		},
	}
}
