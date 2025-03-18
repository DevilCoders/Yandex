package postgres_test

import (
	"fmt"
	"os"
	"testing"

	"github.com/stretchr/testify/assert"
	gormpostgres "gorm.io/driver/postgres"
	"gorm.io/gorm"
	"gorm.io/gorm/logger"

	"a.yandex-team.ru/cdn/cloud_api/pkg/storage"
	"a.yandex-team.ru/cdn/cloud_api/pkg/storage/postgres"
)

func TestPing(t *testing.T) {
	conn, err := constructGorm()
	if err != nil {
		t.Fatal(err)
	}

	result := conn.Exec("select 1")
	if result.Error != nil {
		t.Fatal(result.Error)
	}

	assert.Equal(t, int64(1), result.RowsAffected)
}

func constructCleanStorage() (*postgres.Storage, error) {
	conn, err := constructGorm()
	if err != nil {
		return nil, err
	}

	err = eraseTables(conn)
	if err != nil {
		return nil, err
	}

	return postgres.NewStorage(conn), nil
}

func constructGorm() (*gorm.DB, error) {
	conn, err := gorm.Open(gormpostgres.New(gormpostgres.Config{
		DSN:                  dsn(),
		PreferSimpleProtocol: true,
	}), &gorm.Config{
		Logger: logger.Default.LogMode(logger.Info),
	})
	if err != nil {
		return nil, err
	}

	return conn, nil
}

func dsn() string {
	return fmt.Sprintf("postgresql://%s:%s@localhost:%s?dbname=%s",
		os.Getenv("PG_LOCAL_USER"),
		os.Getenv("PG_LOCAL_PASSWORD"),
		os.Getenv("PG_LOCAL_PORT"),
		os.Getenv("PG_LOCAL_DATABASE"),
	)
}

func eraseTables(conn *gorm.DB) error {
	result := conn.Exec(fmt.Sprintf("DELETE FROM %s", storage.OriginTable))
	if result.Error != nil {
		return result.Error
	}
	result = conn.Exec(fmt.Sprintf("DELETE FROM %s", storage.SecondaryHostnameTable))
	if result.Error != nil {
		return result.Error
	}
	result = conn.Exec(fmt.Sprintf("DELETE FROM %s", storage.ResourceRuleTable))
	if result.Error != nil {
		return result.Error
	}
	result = conn.Exec(fmt.Sprintf("DELETE FROM %s", storage.ResourceTable))
	if result.Error != nil {
		return result.Error
	}
	result = conn.Exec(fmt.Sprintf("DELETE FROM %s", storage.OriginsGroupTable))
	if result.Error != nil {
		return result.Error
	}

	return nil
}
