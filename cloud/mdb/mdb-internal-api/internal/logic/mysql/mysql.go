package mysql

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/internal/compute/compute"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/console"
)

//go:generate ../../../../scripts/mockgen.sh MySQL

type MySQL interface {
	GetDefaultVersions(ctx context.Context) ([]console.DefaultVersion, error)
	GetHostGroupType(ctx context.Context, hostGroupIds []string) (map[string]compute.HostGroupHostType, error)
}
