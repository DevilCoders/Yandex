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

func (lsa *licenseServerAdapter) UpsertLicenseInstance(ctx context.Context, li *license.Instance, tx *sqlx.Tx) error {
	scopedLogger := logging.LoggerWith(
		log.String("id", li.ID),
		log.Any("license_instance", li),
	)

	scopedLogger.Debug("requesting database: upsert_license_instance")

	scopedLogger.Debug("license-server-adapter: transform to db format")
	liYDB, err := li.YDB()
	if err != nil {
		scopedLogger.Error("license-server-adapter: failed to transform to db format", log.Error(err))
		return err
	}

	scopedLogger.Debug("license-server-adapter: executing upsert license instance")
	err = lsa.backend.UpsertLicenseInstance(ctx, liYDB, tx)
	if err != nil {
		scopedLogger.Error("license-server-adapter: failed to upsert license instance", log.Error(err))
		return err
	}

	return nil
}

func (lsa *licenseServerAdapter) GetLicenseInstanceByID(ctx context.Context, id string, tx *sqlx.Tx) (*license.Instance, error) {
	scopedLogger := logging.LoggerWith(log.String("id", id))

	scopedLogger.Debug("requesting database: get_license_instance_by_id")

	scopedLogger.Debug("license-server-adapter: executing get license instance")
	liYDB, err := lsa.backend.GetLicenseInstanceByID(ctx, id, tx)
	if err != nil {
		if errors.Is(err, db_errors.ErrNotFound) {
			err = ErrNotFoundLicenseInstance
		}
		scopedLogger.Error("license-server-adapter: failed to get license instance", log.Error(err))
		return nil, err
	}

	scopedLogger.Debug("license-server-adapter: transform from db format")
	li, err := license.NewInstanceFromYDB(liYDB)
	if err != nil {
		scopedLogger.Error("license-server-adapter: failed to transform from db format", log.Error(err))
		return nil, err
	}

	return li, nil
}

func (lsa *licenseServerAdapter) GetLicenseInstancesByCloudID(ctx context.Context, cloudID string, tx *sqlx.Tx) ([]*license.Instance, error) {
	scopedLogger := logging.LoggerWith(log.String("cloud_id", cloudID))

	scopedLogger.Debug("requesting database: get_license_instances_by_cloud_id")

	scopedLogger.Debug("license-server-adapter: executing get license instances by cloud id")
	lisYDB, err := lsa.backend.GetLicenseInstancesByCloudID(ctx, cloudID, tx)
	if err != nil {
		if errors.Is(err, db_errors.ErrNotFound) {
			err = ErrNotFoundLicenseInstance
		}
		scopedLogger.Error("license-server-adapter: failed to get license instances by cloud id", log.Error(err))
		return nil, err
	}

	scopedLogger.Debug("license-server-adapter: transform from db format")
	lis, err := licenseInstancesFromYDB(lisYDB)
	if err != nil {
		scopedLogger.Error("license-server-adapter: failed to transform from db format", log.Error(err))
		return nil, err
	}

	return lis, nil
}

func (lsa *licenseServerAdapter) GetRecreativeInstances(ctx context.Context, timeNow time.Time, tx *sqlx.Tx) ([]*license.Instance, error) {
	scopedLogger := logging.LoggerWith(log.Time("time_now", timeNow))

	scopedLogger.Debug("requesting database: get_recreative_instances")

	scopedLogger.Debug("license-server-adapter: executing get recreative instances")
	lisYDB, err := lsa.backend.GetRecreativeInstances(ctx, ydb_pkg.UInt64Ts(timeNow), tx)
	if err != nil {
		scopedLogger.Error("license-server-adapter: failed to get recreative instances", log.Error(err))
		return nil, err
	}

	scopedLogger.Debug("license-server-adapter: transform from db format")
	lis, err := licenseInstancesFromYDB(lisYDB)
	if err != nil {
		scopedLogger.Error("license-server-adapter: failed to transform from db format", log.Error(err))
		return nil, err
	}

	return lis, nil
}

func (lsa *licenseServerAdapter) GetPendingInstances(ctx context.Context, timeNow time.Time, tx *sqlx.Tx) ([]*license.Instance, error) {
	scopedLogger := logging.LoggerWith(log.Time("time_now", timeNow))

	scopedLogger.Debug("requesting database: get_pending_instances")

	scopedLogger.Debug("license-server-adapter: executing get pending instances")
	lisYDB, err := lsa.backend.GetPendingInstances(ctx, ydb_pkg.UInt64Ts(timeNow), tx)
	if err != nil {
		scopedLogger.Error("license-server-adapter: failed to get pending instances", log.Error(err))
		return nil, err
	}

	scopedLogger.Debug("license-server-adapter: transform from db format")
	lis, err := licenseInstancesFromYDB(lisYDB)
	if err != nil {
		scopedLogger.Error("license-server-adapter: failed to transform from db format", log.Error(err))
		return nil, err
	}

	return lis, nil
}
