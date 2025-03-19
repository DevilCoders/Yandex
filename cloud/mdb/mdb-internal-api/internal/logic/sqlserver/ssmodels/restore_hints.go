package ssmodels

import (
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/console"
)

type RestoreHints struct {
	console.RestoreHints
	Resources console.RestoreResources
}
