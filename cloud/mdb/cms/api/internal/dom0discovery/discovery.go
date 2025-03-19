package dom0discovery

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/models"
)

//go:generate ../../../../scripts/mockgen.sh Dom0Discovery

type DiscoveryResult struct {
	WellKnown []models.Instance
	Unknown   []string
}

type Dom0Discovery interface {
	Dom0Instances(ctx context.Context, dom0 string) (dscv DiscoveryResult, err error)
}
