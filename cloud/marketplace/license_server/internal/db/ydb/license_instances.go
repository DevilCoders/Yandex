package ydb

import (
	"context"
	"database/sql"
	"fmt"

	"github.com/jmoiron/sqlx"

	"a.yandex-team.ru/cloud/marketplace/license_server/internal/db/ydb/errors"
	"a.yandex-team.ru/cloud/marketplace/pkg/ydb"
)

type LicenseInstance struct {
	ID                string       `db:"id"`
	TemplateID        string       `db:"template_id"`
	TemplateVersionID string       `db:"template_version_id"`
	CloudID           string       `db:"cloud_id"`
	Name              string       `db:"name"`
	StartTime         ydb.UInt64Ts `db:"start_time"`
	EndTime           ydb.UInt64Ts `db:"end_time"`
	CreatedAt         ydb.UInt64Ts `db:"created_at"`
	UpdatedAt         ydb.UInt64Ts `db:"updated_at"`
	State             string       `db:"state"`
}

func (l *licenseServerProvider) GetLicenseInstanceByID(ctx context.Context, id string, tx *sqlx.Tx) (*LicenseInstance, error) {
	const (
		queryTemplate = `
		--!syntax_v1

		DECLARE $id AS Utf8;

		SELECT
			id,
			template_id,
			template_version_id,
			cloud_id,
			name,
			start_time,
			end_time,
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

		licensesTablePath = "meta/license/instances"
	)

	queryBuilder := ydb.NewQueryBuilder(l.Root())

	query := fmt.Sprintf(queryTemplate,
		queryBuilder.TablePath(licensesTablePath),
	)

	namedParams := []interface{}{
		sql.Named("id", id),
	}

	result := []LicenseInstance{}

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

func (l *licenseServerProvider) GetLicenseInstancesByCloudID(ctx context.Context, cloudID string, tx *sqlx.Tx) ([]*LicenseInstance, error) {
	const (
		queryTemplate = `
		--!syntax_v1

		DECLARE $cloud_id AS Utf8;

		SELECT
			id,
			template_id,
			template_version_id,
			cloud_id,
			name,
			start_time,
			end_time,
			created_at,
			updated_at,
			state
		FROM
			%[1]s
		VIEW
			%[2]s
		WHERE
			cloud_id == $cloud_id
			and
			state != "deleted"
			and
			state != "deprecated"
		;`

		licensesTablePath = "meta/license/instances"
		index             = "instances_cloud_id_idx"
	)

	queryBuilder := ydb.NewQueryBuilder(l.Root())

	query := fmt.Sprintf(queryTemplate,
		queryBuilder.TablePath(licensesTablePath),
		queryBuilder.Quote(index),
	)

	namedParams := []interface{}{
		sql.Named("cloud_id", cloudID),
	}

	result := []LicenseInstance{}

	var err error
	if tx != nil {
		err = tx.SelectContext(ctx, &result, query, namedParams...)
	} else {
		err = l.DB().SelectContext(ctx, &result, query, namedParams...)
	}
	if err != nil {
		return nil, err
	}

	pointerResult := make([]*LicenseInstance, 0, len(result))
	for i := range result {
		pointerResult = append(pointerResult, &result[i])
	}

	return pointerResult, nil
}

func (l *licenseServerProvider) UpsertLicenseInstance(ctx context.Context, li *LicenseInstance, tx *sqlx.Tx) error {
	const (
		queryTemplate = `
		--!syntax_v1

		DECLARE $id AS Utf8;
		DECLARE $template_id AS Utf8;
		DECLARE $template_version_id AS Utf8;
		DECLARE $cloud_id AS Utf8;
		DECLARE $name AS Utf8;
		DECLARE $start_time AS Uint64;
		DECLARE $end_time AS Uint64;
		DECLARE $created_at AS Uint64;
		DECLARE $updated_at AS Uint64;
		DECLARE $state AS Utf8;

		UPSERT INTO %[1]s (
			id,
			template_id,
			template_version_id,
			cloud_id,
			name,
			start_time,
			end_time,
			created_at,
			updated_at,
			state
		)
		VALUES (
			$id,
			$template_id,
			$template_version_id,
			$cloud_id,
			$name,
			$start_time,
			$end_time,
			$created_at,
			$updated_at,
			$state
		);`

		licensesTablePath = "meta/license/instances"
	)

	queryBuilder := ydb.NewQueryBuilder(l.Root())

	query := fmt.Sprintf(queryTemplate,
		queryBuilder.TablePath(licensesTablePath),
	)

	namedParams := []interface{}{
		sql.Named("id", li.ID),
		sql.Named("template_id", li.TemplateID),
		sql.Named("template_version_id", li.TemplateVersionID),
		sql.Named("cloud_id", li.CloudID),
		sql.Named("name", li.Name),
		sql.Named("start_time", li.StartTime),
		sql.Named("end_time", li.EndTime),
		sql.Named("created_at", li.CreatedAt),
		sql.Named("updated_at", li.UpdatedAt),
		sql.Named("state", li.State),
	}

	var err error
	if tx != nil {
		_, err = tx.ExecContext(ctx, query, namedParams...)
	} else {
		_, err = l.DB().ExecContext(ctx, query, namedParams...)
	}
	return err
}

func (l *licenseServerProvider) GetRecreativeInstances(ctx context.Context, timeNow ydb.UInt64Ts, tx *sqlx.Tx) ([]*LicenseInstance, error) {
	const (
		queryTemplate = `
		--!syntax_v1

		DECLARE $time_now AS Uint64;

		SELECT
			id,
			template_id,
			template_version_id,
			cloud_id,
			name,
			start_time,
			end_time,
			created_at,
			updated_at,
			state
		FROM
			%[1]s
		VIEW
			%[2]s
		WHERE
			(state == "active" or state == "canceled")
			and
			end_time <= $time_now
		LIMIT 1000
		;`

		licensesTablePath = "meta/license/instances"
		index             = "instances_state_end_time_idx"
	)

	queryBuilder := ydb.NewQueryBuilder(l.Root())

	query := fmt.Sprintf(queryTemplate,
		queryBuilder.TablePath(licensesTablePath),
		queryBuilder.Quote(index),
	)

	namedParams := []interface{}{
		sql.Named("time_now", timeNow),
	}

	result := []LicenseInstance{}

	var err error
	if tx != nil {
		err = tx.SelectContext(ctx, &result, query, namedParams...)
	} else {
		err = l.DB().SelectContext(ctx, &result, query, namedParams...)
	}
	if err != nil {
		return nil, err
	}

	pointerResult := make([]*LicenseInstance, 0, len(result))
	for i := range result {
		pointerResult = append(pointerResult, &result[i])
	}

	return pointerResult, nil
}

func (l *licenseServerProvider) GetPendingInstances(ctx context.Context, timeNow ydb.UInt64Ts, tx *sqlx.Tx) ([]*LicenseInstance, error) {
	const (
		queryTemplate = `
		--!syntax_v1

		DECLARE $time_now AS Uint64;

		SELECT
			id,
			template_id,
			template_version_id,
			cloud_id,
			name,
			start_time,
			end_time,
			created_at,
			updated_at,
			state
		FROM
			%[1]s
		VIEW
			%[2]s
		WHERE
			state == "pending"
			and
			start_time <= $time_now
		LIMIT 1000
		;`

		licensesTablePath = "meta/license/instances"
		index             = "instances_state_start_time_idx"
	)

	queryBuilder := ydb.NewQueryBuilder(l.Root())

	query := fmt.Sprintf(queryTemplate,
		queryBuilder.TablePath(licensesTablePath),
		queryBuilder.Quote(index),
	)

	namedParams := []interface{}{
		sql.Named("time_now", timeNow),
	}

	result := []LicenseInstance{}

	var err error
	if tx != nil {
		err = tx.SelectContext(ctx, &result, query, namedParams...)
	} else {
		err = l.DB().SelectContext(ctx, &result, query, namedParams...)
	}
	if err != nil {
		return nil, err
	}

	pointerResult := make([]*LicenseInstance, 0, len(result))
	for i := range result {
		pointerResult = append(pointerResult, &result[i])
	}

	return pointerResult, nil
}
