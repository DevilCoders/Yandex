package ydb

import (
	"crypto/tls"
	"database/sql"
	"time"

	"github.com/jmoiron/sqlx"
	"golang.org/x/xerrors"

	"a.yandex-team.ru/library/go/core/log"

	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/table"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/ydbsql"
)

type configurator struct {
	endpoint string
	database string
	rootPath string

	auth ydb.Credentials
	tls  *tls.Config

	requestTimeout    time.Duration
	discoveryInterval time.Duration
	// streamTimeout     time.Duration

	connectTimeout time.Duration

	maxConnections       int
	maxIdleConnections   int
	maxDirectConnections int

	connMaxLifetime time.Duration

	logger log.Logger
}

func (c configurator) validate() error {
	if c.logger == nil {
		return xerrors.New("logger must be provided")
	}

	if c.endpoint == "" {
		return xerrors.New("db endpoint should be provided")
	}

	if c.database == "" {
		return xerrors.New("database name is empty")
	}

	return nil
}

func (c configurator) makeYDBDialer() *ydb.Dialer {
	return &ydb.Dialer{
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
}

func (c configurator) openDBWithTableClient(tableClient *table.Client) *sql.DB {
	return sql.OpenDB(
		ydbsql.Connector(
			ydbsql.WithClient(tableClient),
			ydbsql.WithSessionPoolSizeLimit(c.maxConnections),

			// enable server query cache
			ydbsql.WithDefaultExecDataQueryOption(
				table.WithQueryCachePolicy(
					table.WithQueryCachePolicyKeepInCache(),
				),
			),
		),
	)
}

func (c configurator) makeAndSetupSQLDriver(driverDB *sql.DB) *sqlx.DB {
	db := sqlx.NewDb(driverDB, "ydb")

	// TODO: copy-pasted, what the reason for this method?
	db.MapperFunc(func(s string) string {
		return s
	})

	db.SetMaxOpenConns(c.maxConnections)
	db.SetConnMaxLifetime(c.connMaxLifetime)

	maxIdleConnections := c.maxIdleConnections
	if c.maxIdleConnections <= 0 {
		maxIdleConnections = c.maxConnections
	}

	db.SetMaxIdleConns(maxIdleConnections)

	return db
}

var defaultConfigurator = configurator{
	connectTimeout:  time.Second * 20,
	connMaxLifetime: time.Hour,

	requestTimeout: 30 * time.Second,
}

type Option func(c *configurator)

func WithLogger(logger log.Logger) Option {
	return func(c *configurator) {
		c.logger = logger
	}
}

func WithEndpoint(endpoint string) Option {
	return func(c *configurator) {
		c.endpoint = endpoint
	}
}

func WithDatabase(database string) Option {
	return func(c *configurator) {
		c.database = database
	}
}

func WithDBRoot(rootPath string) Option {
	return func(c *configurator) {
		c.rootPath = rootPath
	}
}

func WithTLS(tls *tls.Config) Option {
	return func(c *configurator) {
		c.tls = tls
	}
}

func WithYDBCredentials(auth ydb.Credentials) Option {
	return func(c *configurator) {
		c.auth = auth
	}
}
