package provider

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/search"
)

type SQLServerReindexer struct {
	search search.Docs
}

func NewReindexer(search search.Docs) *SQLServerReindexer {
	return &SQLServerReindexer{
		search: search,
	}
}

func (ssr *SQLServerReindexer) ReindexCluster(ctx context.Context, cid string) error {
	return ssr.search.StoreDocReindex(
		ctx,
		sqlServerService,
		cid,
		searchAttributesExtractor,
	)
}
