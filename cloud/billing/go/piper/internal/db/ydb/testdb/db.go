package testdb

import (
	"context"
	"database/sql"
	"fmt"
	"os"
	"sync"
	"time"

	"github.com/jmoiron/sqlx"

	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/table"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/ydbsql"
)

var ( // From test receipt
	endpointEnv = os.Getenv("YDB_ENDPOINT")
	databaseEnv = os.Getenv("YDB_DATABASE")
)

var (
	DBPath     = readYDBDatabaseName()
	DBEndpoint = readYDBEndpoint()
)

func readYDBEndpoint() string {
	if endpointEnv != "" {
		return endpointEnv
	}
	return "localhost:2135"
}

func readYDBDatabaseName() string {
	if databaseEnv != "" {
		return databaseEnv
	}
	return "local"
}

var (
	testDB  *sqlx.DB
	ydbPool *table.SessionPool
	dbOnce  sync.Once
)

func DB() *sqlx.DB {
	initDB()
	return testDB
}

func YDBPool() *table.SessionPool {
	initDB()
	return ydbPool
}

func initDB() {
	dbOnce.Do(func() {
		dialer := ydb.Dialer{
			DriverConfig: &ydb.DriverConfig{
				Database: DBPath,

				// TODO: Timeouts should be tuned
				RequestTimeout: time.Second * 50,
				StreamTimeout:  time.Second,
			},
			Timeout: 3 * time.Second,
		}
		driver, err := dialer.Dial(context.Background(), DBEndpoint)
		if err != nil {
			panic("YDB inaccessible")
		}
		tableClient := &table.Client{Driver: driver}

		driverDB := sql.OpenDB(ydbsql.Connector(
			ydbsql.WithClient(tableClient),
			// ydbsql.WithSessionPoolSizeLimit(1),
			ydbsql.WithSessionPoolIdleThreshold(time.Second),
			ydbsql.WithMaxRetries(0),
		))
		testDB = sqlx.NewDb(driverDB, "ydb")

		testDB.SetMaxOpenConns(1)
		testDB.SetMaxIdleConns(1)
		testDB.SetConnMaxLifetime(time.Second)

		pCtx, pCancel := context.WithTimeout(context.Background(), time.Millisecond*200)
		defer pCancel()
		err = testDB.PingContext(pCtx)
		if err != nil {
			panic(fmt.Errorf("failed to ping YDB: %w", err))
		}

		ydbPool = &table.SessionPool{
			Builder:       tableClient,
			SizeLimit:     -1, // TODO: limit direct connections
			IdleThreshold: time.Second,
		}
	})
}
