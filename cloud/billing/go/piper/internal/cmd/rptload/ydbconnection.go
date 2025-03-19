package main

import (
	"context"
	"crypto/tls"
	"database/sql"
	"fmt"
	"os"
	"time"

	"github.com/jmoiron/sqlx"

	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/table"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/ydbsql"
)

type ydbConfig struct {
	DBEndpoint string
	DBPath     string
	AuthToken  string
}

var ydbs = map[string]ydbConfig{
	"local": {
		DBEndpoint: "localhost:2135",
		DBPath:     "local",
	},
	"bench": {
		DBEndpoint: "lb.etn9re81cgtsaucel6vm.ydb.mdb.yandexcloud.net:2135",
		DBPath:     "ru-central1/yc.billing.service-cloud/etn9re81cgtsaucel6vm",
		AuthToken:  os.Getenv("YDB_TOKEN"),
	},
}

func DBConnect(ctx context.Context, c ydbConfig) *table.Client {
	dialer := buildDialer(c)
	driver, err := dialer.Dial(ctx, c.DBEndpoint)
	if err != nil {
		panic("YDB inaccessible")
	}
	return &table.Client{Driver: driver}
}

func GetDB(ctx context.Context, tableClient *table.Client) *sqlx.DB {
	driverDB := sql.OpenDB(ydbsql.Connector(
		ydbsql.WithClient(tableClient),
		ydbsql.WithMaxRetries(5),
	))
	db := sqlx.NewDb(driverDB, "ydb")

	//db.SetMaxOpenConns(100)
	//db.SetMaxIdleConns(100)
	db.SetMaxOpenConns(16)
	//db.SetMaxIdleConns(5)
	db.SetConnMaxLifetime(10 * time.Minute)

	pCtx, pCancel := context.WithTimeout(ctx, time.Millisecond*500)
	defer pCancel()
	err := db.PingContext(pCtx)
	if err != nil {
		panic(fmt.Errorf("failed to ping YDB: %w", err))
	}

	return db
}

func GetPool(tableClient *table.Client) *table.SessionPool {
	return &table.SessionPool{
		Builder:       tableClient,
		SizeLimit:     -1, // TODO: limit direct connections
		IdleThreshold: time.Second,
	}
}

func buildDialer(c ydbConfig) ydb.Dialer {
	driverConfig := &ydb.DriverConfig{
		Database: c.DBPath,
		//RequestTimeout:    c.requestTimeout,
		//StreamTimeout:     c.requestTimeout,
		//DiscoveryInterval: c.discoveryInterval,
	}
	dialer := ydb.Dialer{
		DriverConfig: driverConfig,
		//Timeout:      c.connectTimeout,
		Timeout: 3 * time.Second,
	}

	if c.AuthToken != "" {
		driverConfig.Credentials = tokenCreds(c.AuthToken)
		dialer.TLSConfig = &tls.Config{}
	}

	return dialer
}

type tokenCreds string

func (t tokenCreds) Token(context.Context) (string, error) {
	return string(t), nil
}
