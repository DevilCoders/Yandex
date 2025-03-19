package health

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/mdb-pillar-secrets/internal/metadb"
)

type Health struct {
	metaDB metadb.MetaDB
}

func NewHealth(metaDB metadb.MetaDB) Health {
	return Health{metaDB: metaDB}
}

func (h *Health) IsReady(ctx context.Context) error {
	return h.metaDB.IsReady(ctx)
}
