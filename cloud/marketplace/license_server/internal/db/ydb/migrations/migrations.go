package migrations

import (
	"context"
	"database/sql"
	"fmt"
	"sort"

	"a.yandex-team.ru/cloud/marketplace/license_server/internal/db/ydb/errors"
	"a.yandex-team.ru/cloud/marketplace/license_server/pkg/utils"
	"a.yandex-team.ru/cloud/marketplace/pkg/ctxtools"
	"a.yandex-team.ru/cloud/marketplace/pkg/ydb"
)

type Migration struct {
	ID        uint64       `db:"id"`
	Name      string       `db:"name"`
	UP        string       `db:"up"`
	DOWN      string       `db:"down"`
	CreatedAt ydb.UInt64Ts `db:"created_at"`
}

type Migrator struct {
	*ydb.Connector

	migrations []*Migration
}

func quote(variable string) string {
	return fmt.Sprintf("`%s`", variable)
}

func NewMigrator(c *ydb.Connector) *Migrator {
	migrator := &Migrator{
		Connector:  c,
		migrations: make([]*Migration, 0, 1),
	}

	migrator.migrations = append(migrator.migrations, createLicenseTemplatesTable)        // 1
	migrator.migrations = append(migrator.migrations, createLicenseTemplateVersionsTable) // 2
	migrator.migrations = append(migrator.migrations, createLicenseInstancesTable)        // 3
	migrator.migrations = append(migrator.migrations, createLicenseLocksTable)            // 4
	migrator.migrations = append(migrator.migrations, createOperationsTable)              // 5

	sort.Slice(migrator.migrations, func(i, j int) bool {
		return migrator.migrations[i].ID < migrator.migrations[j].ID
	})

	return migrator
}

func (m *Migrator) getLastMigration(ctx context.Context) (uint64, error) {
	const (
		queryTemplate = `
		--!syntax_v1

		SELECT
			id,
			name,
			up,
			down,
			created_at
		FROM
			%[1]s
		ORDER BY id DESC
		LIMIT 1
		;`

		licensesMigrationPath = "migrations"
	)

	queryBuilder := ydb.NewQueryBuilder(m.Root())

	query := fmt.Sprintf(queryTemplate,
		queryBuilder.TablePath(licensesMigrationPath),
	)

	result := []Migration{}

	err := m.DB().SelectContext(ctx, &result, query)
	if err != nil {
		return 0, err
	}

	if len(result) > 1 {
		return 0, errors.ErrExpectedLenOneorLess
	}

	if len(result) == 0 {
		return 0, nil
	}

	return result[0].ID, nil
}

func (m *Migrator) upsertMigration(ctx context.Context, migration *Migration) error {
	const (
		queryTemplate = `
		--!syntax_v1

		DECLARE $id AS Uint64;
		DECLARE $name AS Utf8;
		DECLARE $up AS Utf8;
		DECLARE $down AS Utf8;
		DECLARE $created_at AS Uint64;

		UPSERT INTO %[1]s (
			id,
			name,
			up,
			down,
			created_at
		)
		VALUES (
			$id,
			$name,
			$up,
			$down,
			$created_at
		);`

		licensesMigrationPath = "migrations"
	)

	queryBuilder := ydb.NewQueryBuilder(m.Root())

	query := fmt.Sprintf(queryTemplate,
		queryBuilder.TablePath(licensesMigrationPath),
	)

	namedParams := []interface{}{
		sql.Named("id", migration.ID),
		sql.Named("name", migration.Name),
		sql.Named("up", migration.UP),
		sql.Named("down", migration.DOWN),
		sql.Named("created_at", migration.CreatedAt),
	}

	_, err := m.DB().ExecContext(ctx, query, namedParams...)
	return err
}

func (m *Migrator) deleteMigration(ctx context.Context, id uint64) error {
	const (
		queryTemplate = `
		--!syntax_v1

		DECLARE $id AS Uint64;

		DELETE FROM %[1]s
		WHERE id == $id
		;`

		licensesMigrationPath = "migrations"
	)

	queryBuilder := ydb.NewQueryBuilder(m.Root())

	query := fmt.Sprintf(queryTemplate,
		queryBuilder.TablePath(licensesMigrationPath),
	)

	namedParams := []interface{}{
		sql.Named("id", id),
	}

	_, err := m.DB().ExecContext(ctx, query, namedParams...)
	return err
}

func (m *Migrator) InitUp(ctx context.Context) error {
	scoppedLogger := ctxtools.Logger(ctx)

	scoppedLogger.Info("started up migrations")

	scoppedLogger.Info("init 0 migration number")
	s, err := m.DirectPool().Create(context.Background())
	if err != nil {
		return err
	}

	err = s.ExecuteSchemeQuery(ctx, createMigrationTable.UP)
	if err != nil {
		return err
	}

	return nil
}

func (m *Migrator) InitDown(ctx context.Context) error {
	scoppedLogger := ctxtools.Logger(ctx)

	scoppedLogger.Info("started up migrations")

	scoppedLogger.Info("getting last migration number")
	last, err := m.getLastMigration(ctx)
	if err != nil {
		return err
	}
	scoppedLogger.Info(fmt.Sprintf("last migration is: %v", last))
	if last != 0 {
		return errors.ErrThereOneMoreMigration
	}

	scoppedLogger.Info("init 0 migration number")
	s, err := m.DirectPool().Create(context.Background())
	if err != nil {
		return err
	}

	err = s.ExecuteSchemeQuery(ctx, createMigrationTable.DOWN)
	if err != nil {
		return err
	}

	return nil
}

func (m *Migrator) Up(ctx context.Context) error {
	scoppedLogger := ctxtools.Logger(ctx)

	scoppedLogger.Info("started up migrations")

	scoppedLogger.Info("getting last migration number")
	last, err := m.getLastMigration(ctx)
	if err != nil {
		return err
	}
	scoppedLogger.Info(fmt.Sprintf("last migration is: %v", last))

	s, err := m.DirectPool().Create(context.Background())
	if err != nil {
		return err
	}

	for i := range m.migrations {
		if last >= m.migrations[i].ID {
			continue
		}

		scoppedLogger.Info(fmt.Sprintf("run migration with %v id", m.migrations[i].ID))

		err = s.ExecuteSchemeQuery(ctx, m.migrations[i].UP)
		if err != nil {
			return err
		}
		m.migrations[i].CreatedAt = ydb.UInt64Ts(utils.GetTimeNow())

		err = m.upsertMigration(ctx, m.migrations[i])
		if err != nil {
			return err
		}

		last, err = m.getLastMigration(ctx)
		if err != nil {
			return err
		}
	}
	return nil
}

func (m *Migrator) DownAll(ctx context.Context) error {
	scoppedLogger := ctxtools.Logger(ctx)

	scoppedLogger.Info("started up migrations")

	scoppedLogger.Info("getting last migration number")
	last, err := m.getLastMigration(ctx)
	if err != nil {
		return err
	}
	scoppedLogger.Info(fmt.Sprintf("last migration is: %v", last))

	s, err := m.DirectPool().Create(context.Background())
	if err != nil {
		return err
	}

	for last != 0 {
		for i := range m.migrations {
			if m.migrations[i].ID == last {
				scoppedLogger.Info(fmt.Sprintf("run migration with %v id", m.migrations[i].ID))

				err = s.ExecuteSchemeQuery(ctx, m.migrations[i].DOWN)
				if err != nil {
					return err
				}

				err = m.deleteMigration(ctx, m.migrations[i].ID)
				if err != nil {
					return err
				}

				last, err = m.getLastMigration(ctx)
				if err != nil {
					return err
				}
				break
			}
		}
	}
	return nil
}

func (m *Migrator) Down(ctx context.Context) error {
	scoppedLogger := ctxtools.Logger(ctx)

	scoppedLogger.Info("started up migrations")

	scoppedLogger.Info("getting last migration number")
	last, err := m.getLastMigration(ctx)
	if err != nil {
		return err
	}
	scoppedLogger.Info(fmt.Sprintf("last migration is: %v", last))
	if last == 0 {
		return nil
	}

	s, err := m.DirectPool().Create(context.Background())
	if err != nil {
		return err
	}

	for i := range m.migrations {
		if last == m.migrations[i].ID {
			scoppedLogger.Info(fmt.Sprintf("run down migration with %v id", m.migrations[i].ID))

			err = s.ExecuteSchemeQuery(ctx, m.migrations[i].DOWN)
			if err != nil {
				return err
			}

			err = m.deleteMigration(ctx, m.migrations[i].ID)
			if err != nil {
				return err
			}
			return nil
		}
	}
	return errors.ErrUnexpectedError
}
