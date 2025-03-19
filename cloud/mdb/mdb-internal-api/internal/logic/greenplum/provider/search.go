package provider

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/search"
)

type GreenplumReindexer struct {
	search search.Docs
}

func NewReindexer(search search.Docs) *GreenplumReindexer {
	return &GreenplumReindexer{
		search: search,
	}
}

func (gpr *GreenplumReindexer) ReindexCluster(ctx context.Context, cid string) error {
	return gpr.search.StoreDocReindex(
		ctx,
		greenplumService,
		cid,
		searchAttributesExtractor,
	)
}
