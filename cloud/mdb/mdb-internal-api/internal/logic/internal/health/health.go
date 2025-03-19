package health

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts"
)

//go:generate ../../../../../scripts/mockgen.sh Health

type Health interface {
	Cluster(ctx context.Context, cid string) (clusters.Health, error)
	Hosts(ctx context.Context, fqdns []string) (map[string]hosts.Health, error)
}
