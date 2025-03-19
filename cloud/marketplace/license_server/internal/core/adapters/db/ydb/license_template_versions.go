package ydb

import (
	"context"
	"errors"

	"github.com/jmoiron/sqlx"

	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/model/license"
	db_errors "a.yandex-team.ru/cloud/marketplace/license_server/internal/db/ydb/errors"
	"a.yandex-team.ru/cloud/marketplace/pkg/logging"
	"a.yandex-team.ru/library/go/core/log"
)

func (lsa *licenseServerAdapter) GetLicenseTemplateVersionByID(ctx context.Context, id string, tx *sqlx.Tx) (*license.TemplateVersion, error) {
	scopedLogger := logging.LoggerWith(log.String("id", id))

	scopedLogger.Debug("requesting database: get_license_template_version_by_id")

	scopedLogger.Debug("license-server-adapter: executing get license template version")
	ltvYDB, err := lsa.backend.GetLicenseTemplateVersionByID(ctx, id, tx)
	if err != nil {
		if errors.Is(err, db_errors.ErrNotFound) {
			err = ErrNotFoundLicenseTemplateVersion
		}
		scopedLogger.Error("license-server-adapter: failed to get license template version", log.Error(err))
		return nil, err
	}

	scopedLogger.Debug("license-server-adapter: tranform from db format")
	ltv, err := license.NewTemplateVersionFromYDB(ltvYDB)
	if err != nil {
		scopedLogger.Error("license-server-adapter: failed to tranform from db format", log.Error(err))
		return nil, err
	}

	return ltv, nil
}

func (lsa *licenseServerAdapter) UpsertLicenseTemplateVersion(ctx context.Context, ltv *license.TemplateVersion, tx *sqlx.Tx) error {
	scopedLogger := logging.LoggerWith(
		log.String("id", ltv.ID),
		log.Any("license_template_version", ltv),
	)

	scopedLogger.Debug("requesting database: upsert_license_template_version")

	scopedLogger.Debug("license-server-adapter: transform to db format")
	ltvYDB, err := ltv.YDB()
	if err != nil {
		scopedLogger.Error("license-server-adapter: failed to transform to db format", log.Error(err))
		return err
	}

	scopedLogger.Debug("license-server-adapter: executing upsert license template version")
	err = lsa.backend.UpsertLicenseTemplateVersion(ctx, ltvYDB, tx)
	if err != nil {
		scopedLogger.Error("license-server-adapter: failed to upsert license template version", log.Error(err))
		return err
	}

	return nil
}

func (lsa *licenseServerAdapter) GetPendingLicenseTemplateVersionByLicenseTemplateID(ctx context.Context, licenseTemplateID string, tx *sqlx.Tx) (*license.TemplateVersion, error) {
	scopedLogger := logging.LoggerWith(
		log.String("license_template_version_id", licenseTemplateID),
	)

	scopedLogger.Debug("requesting database: get_pending_license_template_version_by_license_template_id")

	scopedLogger.Debug("license-server-adapter: executing get pending license template version by license template id")
	ltvYDB, err := lsa.backend.GetPendingLicenseTemplateVersionByLicenseTemplateID(ctx, licenseTemplateID, tx)
	if err != nil {
		if errors.Is(err, db_errors.ErrNotFound) {
			err = ErrNotFoundLicenseTemplateVersion
		}
		scopedLogger.Error("license-server-adapter: failed to get pending license template version by license template id", log.Error(err))
		return nil, err
	}

	scopedLogger.Debug("license-server-adapter: tranform from db format")
	ltv, err := license.NewTemplateVersionFromYDB(ltvYDB)
	if err != nil {
		scopedLogger.Error("license-server-adapter: failed to tranform from db format", log.Error(err))
		return nil, err
	}

	return ltv, nil
}

func (lsa *licenseServerAdapter) ListLicenseTemplateVersionByLicenseTemplateID(ctx context.Context, licenseTemplateID string, tx *sqlx.Tx) ([]*license.TemplateVersion, error) {
	scopedLogger := logging.LoggerWith(
		log.String("license_template_version_id", licenseTemplateID),
	)

	scopedLogger.Debug("requesting database: list_license_template_version_by_license_template_id")

	scopedLogger.Debug("license-server-adapter: executing list license template version by license template id")
	ltvsYDB, err := lsa.backend.ListLicenseTemplateVersionByLicenseTemplateID(ctx, licenseTemplateID, tx)
	if err != nil {
		if errors.Is(err, db_errors.ErrNotFound) {
			err = ErrNotFoundLicenseTemplateVersion
		}
		scopedLogger.Error("license-server-adapter: failed to list license template version by license template id", log.Error(err))
		return nil, err
	}

	scopedLogger.Debug("license-server-adapter: tranform from db format")
	ltvs, err := licenseTemplateVersionsFromYDB(ltvsYDB)
	if err != nil {
		scopedLogger.Error("license-server-adapter: failed to tranform from db format", log.Error(err))
		return nil, err
	}

	return ltvs, nil
}
