package healthdbspec

import (
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/core/types"
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/dbspecific/redis"
)

func redisLetGo(ni types.HostNeighboursInfo) bool {
	if defaultHealthCondition(ni) {
		return true
	}
	return ni.SameRolesAlive > 0
}

func registerRedis() []RoleResolver {
	return []RoleResolver{
		{
			func(s string) bool { return s == redis.RoleRedis },
			redisLetGo,
		},
	}
}
