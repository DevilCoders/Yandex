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

func (lsa *licenseServerAdapter) GetLicenseTemplateByID(ctx context.Context, id string, tx *sqlx.Tx) (*license.Template, error) {
	scopedLogger := logging.LoggerWith(
		log.String("id", id),
	)

	scopedLogger.Debug("requesting database: get_license_template_by_id")

	scopedLogger.Debug("license-server-adapter: executing get license template")
	ltYDB, err := lsa.backend.GetLicenseTemplateByID(ctx, id, tx)
	if err != nil {
		if errors.Is(err, db_errors.ErrNotFound) {
			err = ErrNotFoundLicenseTemplate
		}
		scopedLogger.Error("license-server-adapter: failed to get license template", log.Error(err))
		return nil, err
	}

	scopedLogger.Debug("license-server-adapter: transform from db format")
	lt, err := license.NewTemplateFromYDB(ltYDB)
	if err != nil {
		scopedLogger.Error("license-server-adapter: failed to transform from db format", log.Error(err))
		return nil, err
	}

	return lt, nil
}

func (lsa *licenseServerAdapter) ListLicenseTemplatesByTariffID(ctx context.Context, publisherID, productID, tariffID string, tx *sqlx.Tx) ([]*license.Template, error) {
	scopedLogger := logging.LoggerWith(
		log.String("publisher_id", publisherID),
		log.String("product_id", productID),
		log.String("tariff_id", tariffID),
	)

	scopedLogger.Debug("requesting database: list_license_template_by_tariff_id")

	scopedLogger.Debug("license-server-adapter: executing list license template by tariff id")
	ltsYDB, err := lsa.backend.ListLicenseTemplatesByTariffID(ctx, publisherID, productID, tariffID, tx)
	if err != nil {
		if errors.Is(err, db_errors.ErrNotFound) {
			err = ErrNotFoundLicenseTemplate
		}
		scopedLogger.Error("license-server-adapter: failed to list license template", log.Error(err))
		return nil, err
	}

	scopedLogger.Debug("license-server-adapter: transform from db format")
	lts, err := licenseTemplatesFromYDB(ltsYDB)
	if err != nil {
		scopedLogger.Error("license-server-adapter: failed to transform from db format", log.Error(err))
		return nil, err
	}

	return lts, nil
}

func (lsa *licenseServerAdapter) UpsertLicenseTemplate(ctx context.Context, lt *license.Template, tx *sqlx.Tx) error {
	scopedLogger := logging.LoggerWith(
		log.String("id", lt.ID),
		log.Any("license_template", lt),
	)

	scopedLogger.Debug("requesting database: upsert_license_template")

	scopedLogger.Debug("license-server-adapter: transform to db")
	ltYDB, err := lt.YDB()
	if err != nil {
		scopedLogger.Error("license-server-adapter: failed to transform to db", log.Error(err))
		return err
	}

	scopedLogger.Debug("license-server-adapter: executing upsert license template")
	err = lsa.backend.UpsertLicenseTemplate(ctx, ltYDB, tx)
	if err != nil {
		scopedLogger.Error("license-server-adapter: failed to upsert license template", log.Error(err))
		return err
	}

	return nil
}
