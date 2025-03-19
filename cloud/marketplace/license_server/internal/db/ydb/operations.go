package ydb

import (
	"context"
	"database/sql"
	"fmt"

	"github.com/jmoiron/sqlx"

	"a.yandex-team.ru/cloud/marketplace/license_server/internal/db/ydb/errors"
	"a.yandex-team.ru/cloud/marketplace/pkg/ydb"
)

type Operation struct {
	ID          string       `db:"id"`
	Description string       `db:"description"`
	CreatedAt   ydb.UInt64Ts `db:"created_at"`
	CreatedBy   string       `db:"created_by"`
	ModifiedAt  ydb.UInt64Ts `db:"modified_at"`
	Done        bool         `db:"done"`
	Metadata    ydb.AnyJSON  `db:"metadata"`
	Result      ydb.AnyJSON  `db:"result"`
}

func (l *licenseServerProvider) UpsertOperation(ctx context.Context, op *Operation, tx *sqlx.Tx) error {
	const (
		queryTemplate = `
		--!syntax_v1

		DECLARE $id AS Utf8;
		DECLARE $description AS Utf8;
		DECLARE $created_at AS Uint64;
		DECLARE $created_by AS Utf8;
		DECLARE $modified_at AS Uint64;
		DECLARE $done AS bool;
		DECLARE $metadata as Json?;
		DECLARE $result as Json?;

		UPSERT INTO %[1]s (
			id,
			description,
			created_at,
			created_by,
			modified_at,
			done,
			metadata,
			result
		)
		VALUES (
			$id,
			$description,
			$created_at,
			$created_by,
			$modified_at,
			$done,
			$metadata,
			$result
		);`

		licensesTablePath = "meta/operations"
	)

	queryBuilder := ydb.NewQueryBuilder(l.Root())

	query := fmt.Sprintf(queryTemplate,
		queryBuilder.TablePath(licensesTablePath),
	)

	namedParams := []interface{}{
		sql.Named("id", op.ID),
		sql.Named("description", op.Description),
		sql.Named("created_at", op.CreatedAt),
		sql.Named("created_by", op.CreatedBy),
		sql.Named("modified_at", op.ModifiedAt),
		sql.Named("done", op.Done),
		sql.Named("metadata", op.Metadata),
		sql.Named("result", op.Result),
	}

	var err error
	if tx != nil {
		_, err = tx.ExecContext(ctx, query, namedParams...)
	} else {
		_, err = l.DB().ExecContext(ctx, query, namedParams...)
	}
	return err
}

func (l *licenseServerProvider) GetOperation(ctx context.Context, id string, tx *sqlx.Tx) (*Operation, error) {
	const (
		queryTemplate = `
		--!syntax_v1

		DECLARE $id AS Utf8;

		SELECT
			id,
			description,
			created_at,
			created_by,
			modified_at,
			done,
			metadata,
			result
		FROM
			%[1]s
		WHERE
			id == $id
		;`

		licensesTablePath = "meta/operations"
	)

	queryBuilder := ydb.NewQueryBuilder(l.Root())

	query := fmt.Sprintf(queryTemplate,
		queryBuilder.TablePath(licensesTablePath),
	)

	namedParams := []interface{}{
		sql.Named("id", id),
	}

	result := []Operation{}

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
