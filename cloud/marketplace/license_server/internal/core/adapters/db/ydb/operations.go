package ydb

import (
	"context"
	"errors"

	"github.com/jmoiron/sqlx"

	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/model"
	db_errors "a.yandex-team.ru/cloud/marketplace/license_server/internal/db/ydb/errors"
	"a.yandex-team.ru/cloud/marketplace/pkg/logging"
	"a.yandex-team.ru/library/go/core/log"
)

func (lsa *licenseServerAdapter) UpsertOperation(ctx context.Context, op *model.Operation, tx *sqlx.Tx) error {
	scopedLogger := logging.LoggerWith(
		log.String("id", op.ID),
		log.Any("operation", op),
	)

	scopedLogger.Debug("requesting database: upsert_operation")

	scopedLogger.Debug("license-server-adapter: transform to db")
	opYDB, err := op.YDB()
	if err != nil {
		scopedLogger.Error("license-server-adapter: failed to transform to db", log.Error(err))
		return err
	}

	scopedLogger.Debug("license-server-adapter: executing upsert operation")
	err = lsa.backend.UpsertOperation(ctx, opYDB, tx)
	if err != nil {
		scopedLogger.Error("license-server-adapter: failed to upsert operation", log.Error(err))
		return err
	}

	return nil
}

func (lsa *licenseServerAdapter) GetOperation(ctx context.Context, id string, tx *sqlx.Tx) (*model.Operation, error) {
	scopedLogger := logging.LoggerWith(
		log.String("id", id),
	)

	scopedLogger.Debug("requesting database: get_operation")

	scopedLogger.Debug("license-server-adapter: executing get operation")
	opYDB, err := lsa.backend.GetOperation(ctx, id, tx)
	if err != nil {
		if errors.Is(err, db_errors.ErrNotFound) {
			err = ErrNotFoundOperation
		}
		scopedLogger.Error("license-server-adapter: failed to get operation", log.Error(err))
		return nil, err
	}

	scopedLogger.Debug("license-server-adapter: transform from db format")
	op, err := model.NewOperationFromYDB(opYDB)
	if err != nil {
		scopedLogger.Error("license-server-adapter: failed to transform from db format", log.Error(err))
		return nil, err
	}

	return op, nil
}
