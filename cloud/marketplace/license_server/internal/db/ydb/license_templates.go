package ydb

import (
	"context"
	"database/sql"
	"fmt"

	"github.com/jmoiron/sqlx"

	"a.yandex-team.ru/cloud/marketplace/license_server/internal/db/ydb/errors"
	"a.yandex-team.ru/cloud/marketplace/pkg/ydb"
)

type LicenseTemplate struct {
	ID                string       `db:"id"`
	TemplateVersionID ydb.String   `db:"template_version_id"`
	Name              ydb.String   `db:"name"`
	PublisherID       string       `db:"publisher_id"`
	ProductID         string       `db:"product_id"`
	TariffID          string       `db:"tariff_id"`
	LicenseSkuID      ydb.String   `db:"license_sku_id"`
	Period            ydb.String   `db:"period"`
	CreatedAt         ydb.UInt64Ts `db:"created_at"`
	UpdatedAt         ydb.UInt64Ts `db:"updated_at"`
	State             string       `db:"state"`
}

func (l *licenseServerProvider) GetLicenseTemplateByID(ctx context.Context, id string, tx *sqlx.Tx) (*LicenseTemplate, error) {
	const (
		queryTemplate = `
		--!syntax_v1

		DECLARE $id AS Utf8;

		SELECT
			id,
			template_version_id,
			publisher_id,
			product_id,
			tariff_id,
			license_sku_id,
			name,
			period,
			created_at,
			updated_at,
			state
		FROM
			%[1]s
		WHERE
			id == $id
			and
			state != "deleted"
		;`

		licensesTablePath = "meta/license/templates"
	)

	queryBuilder := ydb.NewQueryBuilder(l.Root())

	query := fmt.Sprintf(queryTemplate,
		queryBuilder.TablePath(licensesTablePath),
	)

	namedParams := []interface{}{
		sql.Named("id", id),
	}

	result := []LicenseTemplate{}

	var err error
	if tx != nil {
		err = tx.SelectContext(ctx, &result, query, namedParams...)
	} else {
		err = l.DB().SelectContext(ctx, &result, query, namedParams...)
	}
	if err != nil {
		return nil, err
	}

	if len(result) > 1 {
		return nil, errors.ErrExpectedLenOneorLess
	}

	if len(result) == 0 {
		return nil, errors.ErrNotFound
	}

	return &result[0], nil
}

func (l *licenseServerProvider) ListLicenseTemplatesByTariffID(ctx context.Context, publisherID, productID, tariffID string, tx *sqlx.Tx) ([]*LicenseTemplate, error) {
	const (
		queryTemplate = `
		--!syntax_v1

		DECLARE $publisher_id AS Utf8;
		DECLARE $product_id AS Utf8;
		DECLARE $tariff_id AS Utf8;

		SELECT
			id,
			template_version_id,
			publisher_id,
			product_id,
			tariff_id,
			license_sku_id,
			name,
			period,
			created_at,
			updated_at,
			state
		FROM
			%[1]s
		VIEW
			%[2]s
		WHERE
			publisher_id == $publisher_id
			and
			product_id == $product_id
			and
			tariff_id == $tariff_id
			and
			state != "deleted"
		;`

		licensesTablePath = "meta/license/templates"
		index             = "template_publisher_id_product_id_tariff_id_idx"
	)

	queryBuilder := ydb.NewQueryBuilder(l.Root())

	query := fmt.Sprintf(queryTemplate,
		queryBuilder.TablePath(licensesTablePath),
		queryBuilder.Quote(index),
	)

	namedParams := []interface{}{
		sql.Named("publisher_id", publisherID),
		sql.Named("product_id", productID),
		sql.Named("tariff_id", tariffID),
	}

	result := []LicenseTemplate{}

	var err error
	if tx != nil {
		err = tx.SelectContext(ctx, &result, query, namedParams...)
	} else {
		err = l.DB().SelectContext(ctx, &result, query, namedParams...)
	}
	if err != nil {
		return nil, err
	}

	pointerResult := make([]*LicenseTemplate, 0, len(result))
	for i := range result {
		pointerResult = append(pointerResult, &result[i])
	}

	return pointerResult, nil
}

func (l *licenseServerProvider) UpsertLicenseTemplate(ctx context.Context, lt *LicenseTemplate, tx *sqlx.Tx) error {
	const (
		queryTemplate = `
		--!syntax_v1

		DECLARE $id AS Utf8;
		DECLARE $template_version_id AS Utf8?;
		DECLARE $publisher_id AS Utf8;
		DECLARE $product_id AS Utf8;
		DECLARE $tariff_id AS Utf8;
		DECLARE $period AS Utf8?;
		DECLARE $created_at AS Uint64;
		DECLARE $updated_at AS Uint64;
		DECLARE $state AS Utf8;
		DECLARE $license_sku_id AS Utf8?;
		DECLARE $name AS Utf8?;

		UPSERT INTO %[1]s (
			id,
			template_version_id,
			publisher_id,
			product_id,
			tariff_id,
			license_sku_id,
			name,
			period,
			created_at,
			updated_at,
			state
		)
		VALUES (
			$id,
			$template_version_id,
			$publisher_id,
			$product_id,
			$tariff_id,
			$license_sku_id,
			$name,
			$period,
			$created_at,
			$updated_at,
			$state
		);`

		licensesTablePath = "meta/license/templates"
	)

	queryBuilder := ydb.NewQueryBuilder(l.Root())

	query := fmt.Sprintf(queryTemplate,
		queryBuilder.TablePath(licensesTablePath),
	)

	namedParams := []interface{}{
		sql.Named("id", lt.ID),
		sql.Named("template_version_id", lt.TemplateVersionID),
		sql.Named("publisher_id", lt.PublisherID),
		sql.Named("product_id", lt.ProductID),
		sql.Named("tariff_id", lt.TariffID),
		sql.Named("license_sku_id", lt.LicenseSkuID),
		sql.Named("name", lt.Name),
		sql.Named("period", lt.Period),
		sql.Named("created_at", lt.CreatedAt),
		sql.Named("updated_at", lt.UpdatedAt),
		sql.Named("state", lt.State),
	}

	var err error
	if tx != nil {
		_, err = tx.ExecContext(ctx, query, namedParams...)
	} else {
		_, err = l.DB().ExecContext(ctx, query, namedParams...)
	}
	return err
}
