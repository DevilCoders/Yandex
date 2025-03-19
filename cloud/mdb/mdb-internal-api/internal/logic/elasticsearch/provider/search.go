package provider

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/search"
)

type ElasticSearchReindexer struct {
	search search.Docs
}

func NewReindexer(
	search search.Docs,
) *ElasticSearchReindexer {
	return &ElasticSearchReindexer{
		search: search,
	}
}

func (esr *ElasticSearchReindexer) ReindexCluster(ctx context.Context, cid string) error {
	return esr.search.StoreDocReindex(
		ctx,
		elasticSearchService,
		cid,
		searchAttributesExtractor,
	)
}
