package ydb

import (
	"context"

	"github.com/jmoiron/sqlx"

	"a.yandex-team.ru/cloud/marketplace/license_server/internal/db/ydb"
	"a.yandex-team.ru/cloud/marketplace/pkg/ctxtools"
	"a.yandex-team.ru/library/go/core/log"
)

type licenseServerAdapter struct {
	backend ydb.LicenseServerProvider
}

func NewLicenseServerAdapter(backend ydb.LicenseServerProvider) *licenseServerAdapter {
	return &licenseServerAdapter{
		backend: backend,
	}
}

func (lsa *licenseServerAdapter) CreateTx(ctx context.Context) (*sqlx.Tx, error) {
	scopedLogger := ctxtools.LoggerWith(ctx)

	scopedLogger.Debug("requesting database: create_tx")

	return lsa.backend.CreateTx(ctx)
}

func (lsa *licenseServerAdapter) CommitTx(ctx context.Context, errIn *error, tx *sqlx.Tx) {
	scopedLogger := ctxtools.LoggerWith(ctx)

	var err error
	if errIn != nil {
		err = *errIn
	}

	if err != nil {
		scopedLogger.Debugf("requesting database: rollback: %s", err)
		err = lsa.backend.Rollback(tx)
		if err != nil {
			scopedLogger.Error("requesting database: failed to rollback", log.Error(err))
		}
		return
	}

	scopedLogger.Debug("requesting database: commit")
	err = lsa.backend.Commit(tx)
	if err != nil {
		scopedLogger.Error("requesting database: failed to commit", log.Error(err))
		if errIn != nil {
			*errIn = err
		}
	}
}
