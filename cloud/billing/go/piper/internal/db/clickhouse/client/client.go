package clickhouseclient

import (
	"context"
	"crypto/tls"
	"database/sql"
	"errors"
	"fmt"
	"net/url"
	"strconv"
	"time"

	"github.com/ClickHouse/clickhouse-go"
	"github.com/jmoiron/sqlx"
)

type Connection struct {
	db *sqlx.DB
}

func (c *Connection) DB() *sqlx.DB {
	return c.db
}

func (c *Connection) Close() error {
	return c.db.Close()
}

func (c *Connection) HealthCheck(ctx context.Context) error {
	ctx, cancel := context.WithTimeout(ctx, time.Second)
	defer cancel()
	if err := c.db.PingContext(ctx); err != nil {
		var chExc *clickhouse.Exception
		switch {
		case errors.As(err, &chExc):
			return fmt.Errorf("[%d] %s %s", chExc.Code, chExc.Message, chExc.StackTrace)
		default:
			return ErrPing.Wrap(err)
		}
	}
	return nil
}

type Stats struct {
	DB sql.DBStats
}

func (c *Connection) GetStats() Stats {
	return Stats{
		DB: c.db.Stats(),
	}
}

type Config struct {
	address  string
	port     int
	username string
	password string
	database string
	secure   bool
	compress int
	debug    bool

	maxConnections     int
	maxIdleConnections int
	connMaxLifetime    time.Duration

	ensureAlive bool
}

const tlsConfigName = "default"

func SetTLSConfig(tls *tls.Config) {
	_ = clickhouse.RegisterTLSConfig(tlsConfigName, tls)
}

func NewConfig() *Config {
	return &Config{
		address: "127.0.0.1",
		port:    9000,
		debug:   false,
	}
}

func (c *Config) Build(ctx context.Context) (conn *Connection, resErr error) {
	db, err := sqlx.Connect("clickhouse", c.DSN())
	if err != nil {
		return nil, ErrConnection.Wrap(err)
	}
	defer func() {
		if resErr != nil {
			_ = db.Close()
		}
	}()

	db.MapperFunc(func(s string) string { return s })
	db.SetMaxOpenConns(c.maxConnections)
	maxIdleConnections := c.maxIdleConnections
	if c.maxIdleConnections <= 0 {
		maxIdleConnections = c.maxConnections
	}
	db.SetMaxIdleConns(maxIdleConnections)
	db.SetConnMaxLifetime(c.connMaxLifetime)

	conn = &Connection{db: db}
	if !c.ensureAlive { // for use in cluster
		return conn, ctx.Err()
	}

	err = db.PingContext(ctx)
	if err != nil {
		return nil, ErrPing.Wrap(err)
	}

	return conn, nil
}

func (c *Config) Address(address string) *Config {
	c.address = address
	return c
}

func (c *Config) Port(port int) *Config {
	c.port = port
	return c
}

func (c *Config) Username(username string) *Config {
	c.username = username
	return c
}

func (c *Config) Password(password string) *Config {
	c.password = password
	return c
}

func (c *Config) Database(database string) *Config {
	c.database = database
	return c
}

func (c *Config) Secure(v bool) *Config {
	c.secure = v
	return c
}

func (c *Config) Compress(v int) *Config {
	c.compress = v
	return c
}

func (c *Config) Alive() *Config {
	c.ensureAlive = true
	return c
}

func (c *Config) Debug(debug bool) *Config {
	c.debug = debug
	return c
}

func (c *Config) DSN() string {
	query := url.Values{}
	query.Set("debug", strconv.FormatBool(c.debug))
	query.Set("compress", strconv.Itoa(c.compress))
	query.Set("username", c.username)
	query.Set("password", c.password)
	query.Set("database", c.database)
	if c.secure {
		query.Set("secure", "true")
		query.Set("tls_config", tlsConfigName)
	}
	dsn := url.URL{
		Scheme:   "tcp",
		Host:     fmt.Sprintf("%s:%d", c.address, c.port),
		RawQuery: query.Encode(),
	}
	return dsn.String()
}
