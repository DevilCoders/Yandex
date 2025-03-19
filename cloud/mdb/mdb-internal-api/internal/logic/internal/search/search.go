package search

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
)

//go:generate ../../../../../scripts/mockgen.sh Docs

type AttributesExtractor func(cluster clusters.Cluster) (map[string]interface{}, error)

type Docs interface {
	StoreDoc(ctx context.Context, service, folderExtID, cloudExtID string, operation operations.Operation, attributesExtractor AttributesExtractor) error
	StoreDocDelete(ctx context.Context, service, folderExtID, cloudExtID string, operation operations.Operation) error
	StoreDocReindex(ctx context.Context, service, cid string, attributesExtractor AttributesExtractor) error
}
