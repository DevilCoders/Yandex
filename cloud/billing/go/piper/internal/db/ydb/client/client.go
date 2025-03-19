package ydbclient

import (
	"context"
	"crypto/tls"
	"database/sql"
	"fmt"
	"time"

	"github.com/jmoiron/sqlx"

	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/table"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/ydbsql"
)

type Connections struct {
	directPool *table.SessionPool
	db         *sqlx.DB
}

func (c *Connections) DirectPool() *table.SessionPool {
	return c.directPool
}

func (c *Connections) DB() *sqlx.DB {
	return c.db
}

func (c *Connections) HealthCheck(ctx context.Context) error {
	ctx, cancel := context.WithTimeout(ctx, time.Second)
	defer cancel()
	if err := c.db.PingContext(ctx); err != nil {
		return fmt.Errorf("ping error: %w", err)
	}
	return nil
}

type Stats struct {
	DB     sql.DBStats
	Direct table.SessionPoolStats
}

func (c *Connections) GetStats() Stats {
	return Stats{
		DB:     c.db.Stats(),
		Direct: c.directPool.Stats(),
	}
}

func (c *Connections) Close() error {
	closeCtx, cancel := context.WithTimeout(context.Background(), time.Second)
	defer cancel()
	ce := closeError{}
	ce.directErr = c.directPool.Close(closeCtx)
	ce.dbErr = c.db.Close()
	if ce.directErr != nil && ce.dbErr != nil {
		return ce
	}
	return nil
}

type Config struct {
	addr     string
	database string

	auth ydb.Credentials
	tls  *tls.Config

	connectTimeout    time.Duration
	requestTimeout    time.Duration
	discoveryInterval time.Duration

	maxConnections       int
	maxIdleConnections   int
	maxDirectConnections int
	connMaxLifetime      time.Duration
}

func NewConfig() *Config {
	return &Config{
		connectTimeout:  time.Second * 5,
		connMaxLifetime: time.Hour,
	}
}

func (c *Config) Build(ctx context.Context) (con *Connections, resErr error) {
	dialer := ydb.Dialer{
		DriverConfig: &ydb.DriverConfig{
			Database:          c.database,
			Credentials:       c.auth,
			RequestTimeout:    c.requestTimeout,
			StreamTimeout:     c.requestTimeout,
			DiscoveryInterval: c.discoveryInterval,
		},
		TLSConfig: c.tls,
		Timeout:   c.connectTimeout,
	}
	driver, err := dialer.Dial(ctx, c.addr)
	if err != nil {
		return nil, ErrConnection.Wrap(err)
	}
	defer func() {
		if resErr != nil {
			_ = driver.Close()
		}
	}()

	tableClient := &table.Client{Driver: driver}
	driverDB := sql.OpenDB(ydbsql.Connector(
		ydbsql.WithClient(tableClient),
		ydbsql.WithSessionPoolSizeLimit(c.maxConnections*2),
		// enable server query cache
		ydbsql.WithDefaultExecDataQueryOption(table.WithQueryCachePolicy(table.WithQueryCachePolicyKeepInCache())),
	))
	defer func() {
		if resErr != nil {
			_ = driverDB.Close()
		}
	}()

	db := sqlx.NewDb(driverDB, "ydb")
	db.MapperFunc(func(s string) string { return s })

	db.SetMaxOpenConns(c.maxConnections)
	maxIdleConnections := c.maxIdleConnections
	if c.maxIdleConnections <= 0 {
		maxIdleConnections = c.maxConnections
	}
	db.SetMaxIdleConns(maxIdleConnections)
	db.SetConnMaxLifetime(c.connMaxLifetime)

	err = db.PingContext(ctx)
	if err != nil {
		return nil, ErrPing.Wrap(err)
	}

	directPool := &table.SessionPool{
		Builder:       tableClient,
		SizeLimit:     c.maxDirectConnections,
		IdleThreshold: time.Second,
	}

	return &Connections{db: db, directPool: directPool}, nil
}

func (c *Config) DB(hostPort string, name string) *Config {
	c.addr = hostPort
	c.database = name
	return c
}

func (c *Config) Credentials(auth ydb.Credentials) *Config {
	c.auth = auth
	return c
}

func (c *Config) TLS(tls *tls.Config) *Config {
	c.tls = tls
	return c
}

func (c *Config) ConnectTimeout(d time.Duration) *Config {
	c.connectTimeout = d
	return c
}

func (c *Config) RequestTimeout(d time.Duration) *Config {
	c.requestTimeout = d
	return c
}

func (c *Config) DiscoveryInterval(d time.Duration) *Config {
	c.discoveryInterval = d
	return c
}

func (c *Config) MaxConnections(i int) *Config {
	c.maxConnections = i
	return c
}

func (c *Config) MaxIdleConnections(i int) *Config {
	c.maxIdleConnections = i
	return c
}

func (c *Config) MaxDirectConnections(i int) *Config {
	c.maxDirectConnections = i
	return c
}

func (c *Config) ConnMaxLifetime(d time.Duration) *Config {
	c.connMaxLifetime = d
	return c
}
