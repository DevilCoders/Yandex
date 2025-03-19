package clusters

import (
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts"
)

type Roles struct {
	Possible []hosts.Role
	Main     hosts.Role
}
