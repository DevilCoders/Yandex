package console

import (
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
)

type FolderStats struct {
	Clusters map[clusters.Type]int64
}
