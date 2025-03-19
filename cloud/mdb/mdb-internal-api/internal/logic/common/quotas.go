package common

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/metadb"
)

type BatchUpdateResources struct {
	CPU      optional.Float64 // Cores count
	GPU      optional.Int64   // Cores count
	Memory   optional.Int64   // Bytes
	SSDSpace optional.Int64   // Bytes
	HDDSpace optional.Int64   // Bytes
	Clusters optional.Int64   // Count
}

type Quotas interface {
	Get(ctx context.Context, cloudExtID string) (metadb.Cloud, error)
	BatchUpdateMetric(ctx context.Context, cloudExtID string, quota BatchUpdateResources) error
}
