package ydb

import (
	"context"
	"database/sql"
	"fmt"

	"github.com/jmoiron/sqlx"

	"a.yandex-team.ru/cloud/marketplace/license_server/internal/db/ydb/errors"
	"a.yandex-team.ru/cloud/marketplace/pkg/ydb"
)

type LicenseLock struct {
	ID             string       `db:"id"`
	InstanceID     string       `db:"instance_id"`
	ResourceLockID string       `db:"resource_lock_id"`
	StartTime      ydb.UInt64Ts `db:"start_time"`
	EndTime        ydb.UInt64Ts `db:"end_time"`
	CreatedAt      ydb.UInt64Ts `db:"created_at"`
	UpdatedAt      ydb.UInt64Ts `db:"updated_at"`
	State          string       `db:"state"`
}

func (l *licenseServerProvider) GetLicenseLockByInstanceID(ctx context.Context, instanceID string, tx *sqlx.Tx) (*LicenseLock, error) {
	const (
		queryTemplate = `
		--!syntax_v1

		DECLARE $instance_id AS Utf8;

		SELECT
			id,
			instance_id,
			resource_lock_id,
			start_time,
			end_time,
			state
		FROM
			%[1]s
		VIEW
			%[2]s
		WHERE
			instance_id == $instance_id
			and
			state == "locked"
		;`

		licensesTablePath = "meta/license/locks"
		index             = "locks_instance_id_idx"
	)

	queryBuilder := ydb.NewQueryBuilder(l.Root())

	query := fmt.Sprintf(queryTemplate,
		queryBuilder.TablePath(licensesTablePath),
		queryBuilder.Quote(index),
	)

	namedParams := []interface{}{
		sql.Named("instance_id", instanceID),
	}

	result := []LicenseLock{}

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

func (l *licenseServerProvider) UnlockLicenseLocksByLicenseInstanceID(ctx context.Context, licenseInstanceID string, endTime ydb.UInt64Ts, tx *sqlx.Tx) error {
	const (
		queryTemplate = `
		--!syntax_v1

		DECLARE $instance_id AS Utf8;
		DECLARE $end_time AS Uint64;

		UPDATE %[1]s
		SET
			state = "unlocked",
			end_time = $end_time,
			updated_at = $end_time
		WHERE
			instance_id == $instance_id
			and
			state == "locked"
		;`

		licensesTablePath = "meta/license/locks"
	)

	queryBuilder := ydb.NewQueryBuilder(l.Root())

	query := fmt.Sprintf(queryTemplate,
		queryBuilder.TablePath(licensesTablePath),
	)

	namedParams := []interface{}{
		sql.Named("instance_id", licenseInstanceID),
		sql.Named("end_time", endTime),
	}

	var err error
	if tx != nil {
		_, err = tx.ExecContext(ctx, query, namedParams...)
	} else {
		_, err = l.DB().ExecContext(ctx, query, namedParams...)
	}
	return err
}

func (l *licenseServerProvider) UpsertLicenseLock(ctx context.Context, ll *LicenseLock, tx *sqlx.Tx) error {
	const (
		queryTemplate = `
		--!syntax_v1

		DECLARE $id AS Utf8;
		DECLARE $instance_id AS Utf8;
		DECLARE $resource_lock_id AS Utf8;
		DECLARE $start_time AS Uint64;
		DECLARE $end_time AS Uint64;
		DECLARE $created_at AS Uint64;
		DECLARE $updated_at AS Uint64;
		DECLARE $state AS Utf8;

		UPSERT INTO %[1]s (
			id,
			instance_id,
			resource_lock_id,
			start_time,
			end_time,
			created_at,
			updated_at,
			state
		)
		VALUES (
			$id,
			$instance_id,
			$resource_lock_id,
			$start_time,
			$end_time,
			$created_at,
			$updated_at,
			$state
		);`

		licensesTablePath = "meta/license/locks"
	)

	queryBuilder := ydb.NewQueryBuilder(l.Root())

	query := fmt.Sprintf(queryTemplate,
		queryBuilder.TablePath(licensesTablePath),
	)

	namedParams := []interface{}{
		sql.Named("id", ll.ID),
		sql.Named("instance_id", ll.InstanceID),
		sql.Named("resource_lock_id", ll.ResourceLockID),
		sql.Named("start_time", ll.StartTime),
		sql.Named("end_time", ll.EndTime),
		sql.Named("created_at", ll.CreatedAt),
		sql.Named("updated_at", ll.UpdatedAt),
		sql.Named("state", ll.State),
	}

	var err error
	if tx != nil {
		_, err = tx.ExecContext(ctx, query, namedParams...)
	} else {
		_, err = l.DB().ExecContext(ctx, query, namedParams...)
	}
	return err
}
