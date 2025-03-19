package postgresql

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/internal/compute/compute"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/postgresql/pgmodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/console"
)

type PostgreSQL interface {
	Database(ctx context.Context, cid, name string) (pgmodels.Database, error)
	Databases(ctx context.Context, cid string, limit, offset int64) ([]pgmodels.Database, error)

	GetDefaultVersions(ctx context.Context) ([]console.DefaultVersion, error)
	GetHostGroupType(ctx context.Context, hostGroupIds []string) (map[string]compute.HostGroupHostType, error)
}
