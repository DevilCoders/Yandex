package health

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/internal/datatransfer"
	"a.yandex-team.ru/cloud/mdb/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/logs-api/internal/logsdb"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type Health struct {
	metaDB              metadb.MetaDB
	datatransferService datatransfer.DataTransferService
	logsdb              logsdb.Backend
}

func NewHealth(metaDB metadb.MetaDB, datatransferService datatransfer.DataTransferService, logsdb logsdb.Backend) Health {
	return Health{
		metaDB:              metaDB,
		datatransferService: datatransferService,
		logsdb:              logsdb,
	}
}

func (h *Health) IsReady(ctx context.Context) error {
	if err := h.metaDB.IsReady(ctx); err != nil {
		return xerrors.Errorf("mdb metadb ready: %s", err)
	}

	if err := h.datatransferService.IsReady(ctx); err != nil {
		return xerrors.Errorf("datatransfer api ready: %s", err)
	}

	if err := h.logsdb.IsReady(ctx); err != nil {
		return xerrors.Errorf("logsdb ready: %s", err)
	}

	return nil
}
