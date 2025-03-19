package ydb

import (
	"context"
	"errors"
	"time"

	"github.com/jmoiron/sqlx"

	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/model/license"
	db_errors "a.yandex-team.ru/cloud/marketplace/license_server/internal/db/ydb/errors"
	"a.yandex-team.ru/cloud/marketplace/pkg/logging"
	ydb_pkg "a.yandex-team.ru/cloud/marketplace/pkg/ydb"
	"a.yandex-team.ru/library/go/core/log"
)

func (lsa *licenseServerAdapter) UpsertLicenseLock(ctx context.Context, ll *license.Lock, tx *sqlx.Tx) error {
	scopedLogger := logging.LoggerWith(
		log.String("id", ll.ID),
		log.Any("license_lock", ll),
	)

	scopedLogger.Debug("requesting database: upsert_license_lock")

	scopedLogger.Debug("license-server-adapter: transform to db format")
	llYDB, err := ll.YDB()
	if err != nil {
		scopedLogger.Error("license-server-adapter: failed to transform to db format", log.Error(err))
		return err
	}

	scopedLogger.Debug("license-server-adapter: executing upsert license lock")
	err = lsa.backend.UpsertLicenseLock(ctx, llYDB, tx)
	if err != nil {
		scopedLogger.Error("license-server-adapter: failed to upsert license lock", log.Error(err))
		return err
	}

	return nil
}

func (lsa *licenseServerAdapter) GetLicenseLockByInstanceID(ctx context.Context, licenseInstanceID string, tx *sqlx.Tx) (*license.Lock, error) {
	scopedLogger := logging.LoggerWith(
		log.String("license_instance_id", licenseInstanceID),
	)

	scopedLogger.Debug("requesting database: get_license_lock_by_main_ids")

	scopedLogger.Debug("license-server-adapter: executing get license lock by main ids")
	llYDB, err := lsa.backend.GetLicenseLockByInstanceID(ctx, licenseInstanceID, tx)
	if err != nil {
		if errors.Is(err, db_errors.ErrNotFound) {
			err = ErrNotFoundLicenseLock
		}
		scopedLogger.Error("license-server-adapter: failed to get license lock by main ids", log.Error(err))
		return nil, err
	}

	scopedLogger.Debug("license-server-adapter: transform from db format")
	ll, err := license.NewLockFromYDB(llYDB)
	if err != nil {
		scopedLogger.Error("license-server-adapter: failed to transform license lock from db format", log.Error(err))
		return nil, err
	}

	return ll, nil
}

func (lsa *licenseServerAdapter) UnlockLicenseLocksByLicenseInstanceID(ctx context.Context, billingAccountID string, endTime time.Time, tx *sqlx.Tx) error {
	scopedLogger := logging.LoggerWith(
		log.String("billing_account_id", billingAccountID),
	)

	scopedLogger.Debug("requesting database: unlock_license_locks_by_billing_id")

	scopedLogger.Debug("license-server-adapter: executing unlock license locks by billing id")
	err := lsa.backend.UnlockLicenseLocksByLicenseInstanceID(ctx, billingAccountID, ydb_pkg.UInt64Ts(endTime), tx)
	if err != nil {
		scopedLogger.Error("license-server-adapter: failed to unlock license locks by billing id", log.Error(err))
		return err
	}

	return nil
}
