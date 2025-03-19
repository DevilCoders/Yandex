package healthdbspec

import (
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/dbspecific/greenplum"
)

func registerGreenplum() []RoleResolver {
	return []RoleResolver{
		{
			func(s string) bool { return s == greenplum.RoleMaster || s == greenplum.RoleSegments },
			defaultNoQuorum,
		},
	}
}
