package provider

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/search"
	"a.yandex-team.ru/library/go/core/log"
)

type KafkaReindexer struct {
	search search.Docs
	l      log.Logger
}

func NewReindexer(
	search search.Docs,
) *KafkaReindexer {
	return &KafkaReindexer{
		search: search,
	}
}

func (kfr *KafkaReindexer) ReindexCluster(ctx context.Context, cid string) error {
	return kfr.search.StoreDocReindex(
		ctx,
		kafkaService,
		cid,
		searchAttributesExtractor,
	)
}
