package support

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/support/clmodels"
)

//go:generate ../../../../scripts/mockgen.sh Support

type Support interface {
	ResolveCluster(ctx context.Context, cid string) (clmodels.ClusterResult, error)
}
