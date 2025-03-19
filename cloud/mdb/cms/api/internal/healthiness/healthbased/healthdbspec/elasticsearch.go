package healthdbspec

import (
	"a.yandex-team.ru/cloud/mdb/mdb-health/pkg/dbspecific/elasticsearch"
)

func registerElasticsearch() []RoleResolver {
	return []RoleResolver{
		{
			func(s string) bool { return s == elasticsearch.RoleData },
			defaultNoQuorum,
		},
	}
}
