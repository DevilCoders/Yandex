package healthdbspec

import (
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/dbspecific/mongodb"
)

func registerMongodb() []RoleResolver {
	return []RoleResolver{
		{
			func(s string) bool { return s == mongodb.RoleMongos },
			defaultNoQuorum,
		},
	}
}
