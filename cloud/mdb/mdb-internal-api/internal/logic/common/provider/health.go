package provider

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/internal/fs"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/common"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logsdb"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/metadb"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type Health struct {
	metaDB metadb.Backend
	ldb    logsdb.Backend
	slbc   fs.FileWatcher
}

var _ common.Health = &Health{}

func NewHealth(metaDB metadb.Backend, ldb logsdb.Backend, slbc fs.FileWatcher) *Health {
	return &Health{metaDB: metaDB, ldb: ldb, slbc: slbc}
}

// IsReady checks is service is ready to serve requests
func (h *Health) IsReady(ctx context.Context) error {
	if err := h.metaDB.IsReady(ctx); err != nil {
		return err
	}

	if h.slbc.Exists() {
		return xerrors.Errorf("closed from slb")
	}

	// TODO: answer with detailed status (for example someone might be ok with inaccessible logdb)
	/*if err := h.ldb.IsReady(ctx); err != nil {
		return err
	}

	if err := h.h.Auth.IsReady(ctx); err != nil {
		return err
	}*/

	return nil
}
