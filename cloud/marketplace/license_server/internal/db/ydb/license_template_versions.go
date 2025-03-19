package ydb

import (
	"context"
	"database/sql"
	"fmt"

	"github.com/jmoiron/sqlx"

	"a.yandex-team.ru/cloud/marketplace/license_server/internal/db/ydb/errors"
	"a.yandex-team.ru/cloud/marketplace/pkg/ydb"
)

type GetLicenseTemplateVersionsParams struct {
	IDs []string
}

type LicenseTemplateVersion struct {
	ID           string                  `db:"id"`
	TemplateID   string                  `db:"template_id"`
	Price        ydb.MapStringStringJSON `db:"price"`
	LicenseSkuID ydb.String              `db:"license_sku_id"`
	Name         string                  `db:"name"`
	Period       string                  `db:"period"`
	CreatedAt    ydb.UInt64Ts            `db:"created_at"`
	UpdatedAt    ydb.UInt64Ts            `db:"updated_at"`
	State        string                  `db:"state"`
}

func (l *licenseServerProvider) GetLicenseTemplateVersionByID(ctx context.Context, id string, tx *sqlx.Tx) (*LicenseTemplateVersion, error) {
	const (
		queryTemplate = `
		--!syntax_v1

		DECLARE $id AS Utf8;

		SELECT
			id,
			template_id,
			price,
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

		licensesTablePath = "meta/license/template_versions"
	)

	queryBuilder := ydb.NewQueryBuilder(l.Root())

	query := fmt.Sprintf(queryTemplate,
		queryBuilder.TablePath(licensesTablePath),
	)

	namedParams := []interface{}{
		sql.Named("id", id),
	}

	result := []LicenseTemplateVersion{}

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

func (l *licenseServerProvider) GetPendingLicenseTemplateVersionByLicenseTemplateID(ctx context.Context, licenseTemplateID string, tx *sqlx.Tx) (*LicenseTemplateVersion, error) {
	const (
		queryTemplate = `
		--!syntax_v1

		DECLARE $template_id AS Utf8;

		SELECT
			id,
			template_id,
			price,
			license_sku_id,
			name,
			period,
			created_at,
			updated_at,
			state
		FROM
			%[1]s
		VIEW %[2]s
		WHERE
			template_id == $template_id
			and
			state == "pending"
		;`

		licensesTablePath = "meta/license/template_versions"
		index             = "template_versions_template_id_idx"
	)

	queryBuilder := ydb.NewQueryBuilder(l.Root())

	query := fmt.Sprintf(queryTemplate,
		queryBuilder.TablePath(licensesTablePath),
		queryBuilder.Quote(index),
	)

	namedParams := []interface{}{
		sql.Named("template_id", licenseTemplateID),
	}

	result := []LicenseTemplateVersion{}

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

func (l *licenseServerProvider) ListLicenseTemplateVersionByLicenseTemplateID(ctx context.Context, licenseTemplateID string, tx *sqlx.Tx) ([]*LicenseTemplateVersion, error) {
	const (
		queryTemplate = `
		--!syntax_v1

		DECLARE $template_id AS Utf8;

		SELECT
			id,
			template_id,
			price,
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
			template_id == $template_id
			and
			state != "deleted"
		;`

		licensesTablePath = "meta/license/template_versions"
		index             = "template_versions_template_id_idx"
	)

	queryBuilder := ydb.NewQueryBuilder(l.Root())

	query := fmt.Sprintf(queryTemplate,
		queryBuilder.TablePath(licensesTablePath),
		queryBuilder.Quote(index),
	)

	namedParams := []interface{}{
		sql.Named("template_id", licenseTemplateID),
	}

	result := []LicenseTemplateVersion{}

	var err error
	if tx != nil {
		err = tx.SelectContext(ctx, &result, query, namedParams...)
	} else {
		err = l.DB().SelectContext(ctx, &result, query, namedParams...)
	}
	if err != nil {
		return nil, err
	}

	pointerResult := make([]*LicenseTemplateVersion, 0, len(result))
	for i := range result {
		pointerResult = append(pointerResult, &result[i])
	}

	return pointerResult, nil
}

func (l *licenseServerProvider) UpsertLicenseTemplateVersion(ctx context.Context, ltv *LicenseTemplateVersion, tx *sqlx.Tx) error {
	const (
		queryTemplate = `
		--!syntax_v1

		DECLARE $id AS Utf8;
		DECLARE $template_id AS Utf8;
		DECLARE $price AS Json?;
		DECLARE $period AS Utf8;
		DECLARE $created_at AS Uint64;
		DECLARE $updated_at AS Uint64;
		DECLARE $state AS Utf8;
		DECLARE $license_sku_id AS Utf8?;
		DECLARE $name AS Utf8;

		UPSERT INTO %[1]s (
			id,
			template_id,
			price,
			license_sku_id,
			name,
			period,
			created_at,
			updated_at,
			state
		)
		VALUES (
			$id,
			$template_id,
			$price,
			$license_sku_id,
			$name,
			$period,
			$created_at,
			$updated_at,
			$state
		);`

		licensesTablePath = "meta/license/template_versions"
	)

	queryBuilder := ydb.NewQueryBuilder(l.Root())

	query := fmt.Sprintf(queryTemplate,
		queryBuilder.TablePath(licensesTablePath),
	)

	namedParams := []interface{}{
		sql.Named("id", ltv.ID),
		sql.Named("template_id", ltv.TemplateID),
		sql.Named("price", ltv.Price),
		sql.Named("license_sku_id", ltv.LicenseSkuID),
		sql.Named("name", ltv.Name),
		sql.Named("period", ltv.Period),
		sql.Named("created_at", ltv.CreatedAt),
		sql.Named("updated_at", ltv.UpdatedAt),
		sql.Named("state", ltv.State),
	}

	var err error
	if tx != nil {
		_, err = tx.ExecContext(ctx, query, namedParams...)
	} else {
		_, err = l.DB().ExecContext(ctx, query, namedParams...)
	}
	return err
}
