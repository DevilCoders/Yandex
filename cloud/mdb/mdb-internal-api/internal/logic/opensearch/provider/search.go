package provider

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/search"
)

type OpenSearchReindexer struct {
	search search.Docs
}

func NewReindexer(search search.Docs) *OpenSearchReindexer {
	return &OpenSearchReindexer{
		search: search,
	}
}

func (esr *OpenSearchReindexer) ReindexCluster(ctx context.Context, cid string) error {
	return esr.search.StoreDocReindex(
		ctx,
		openSearchService,
		cid,
		searchAttributesExtractor,
	)
}
