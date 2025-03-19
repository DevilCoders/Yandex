package healthdbspec

import (
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/dbspecific/clickhouse"
)

func chLetGo(ni types.HostNeighboursInfo) bool {
	if defaultHealthCondition(ni) {
		return true
	}
	return ni.SameRolesAlive > 0 && ni.SameRolesTotal > 0
}

func registerZK() []RoleResolver {
	return []RoleResolver{
		{
			func(s string) bool { return s == clickhouse.RoleCH },
			chLetGo,
		},
	}
}
