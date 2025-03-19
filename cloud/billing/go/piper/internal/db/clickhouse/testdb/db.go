package testdb

import (
	"context"
	"fmt"
	"os"
	"strconv"
	"sync"
	"time"

	"github.com/jmoiron/sqlx"

	clickhouseclient "a.yandex-team.ru/cloud/billing/go/piper/internal/db/clickhouse/client"
)

var (
	testDB *sqlx.DB
	dbOnce sync.Once
)

func getClickhousePort() int {
	portStr := os.Getenv("RECIPE_CLICKHOUSE_NATIVE_PORT")
	if portStr == "" {
		return 9000
	}
	port, _ := strconv.Atoi(portStr)
	return port
}

func initDB() {
	dbOnce.Do(func() {
		ctx, cancel := context.WithTimeout(context.Background(), time.Second*5)
		defer cancel()

		config := clickhouseclient.NewConfig().Port(getClickhousePort())

		conn, err := config.Build(ctx)
		if err != nil {
			panic(fmt.Errorf("failed to connect Clickhouse: %w", err))
		}
		testDB = conn.DB()
	})
}

func DB() *sqlx.DB {
	initDB()
	return testDB
}
