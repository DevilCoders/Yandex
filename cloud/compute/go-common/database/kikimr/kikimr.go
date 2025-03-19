// This package handles database and table name matching
// from short, user-defined names (like "db1" and "table1")
// to long, kikimr-compatible names (like [Root/Home/db1/table1]).
//
// Database prefixes and connection endpoints are acquired
// from config. Table names must be provided by user.
//
// Usage:
//	conf := Config{
//		"db1": DatabaseConfig{
//			Host: "host1",
//			Root: "Root/",
//		},
//	}
// 	p, err := NewDatabaseProvider(conf)
//	if err != nil {
//		t.Fatal(err)
//	}
//
//	if err = p.CreateFakeTables("db1/t1", "db1/t2"); err != nil {
//		t.Fatal(err)
//	}
//	// Or
//	if err = p.Verify("db1/t1", "db1/t2"); err != nil {
//		t.Fatal(err)
//	}
//
//	fmt.Println(p.GetDatabase("db1").GetTable("t1").GetName())
//	// Output: [Root/t1]
//    	p.GetTable("db1/t2").GetName()
//	// Output: [Root/Home/t2]
package kikimr

import (
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/table"
	"context"
	"crypto/tls"
	"database/sql"
	"errors"
	"fmt"
	"io/ioutil"
	"os/user"
	"path"
	"strings"
	"time"

	"go.uber.org/zap"

	"a.yandex-team.ru/cloud/compute/go-common/logging"
	"a.yandex-team.ru/cloud/compute/go-common/logging/ctxlog"
	"a.yandex-team.ru/cloud/compute/go-common/tracing"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/ydbsql"
)

const (
	defaultMaxConns = 256
)

var log = logging.GetLogger("kikimr")

// Generic interface for kikimr table
type Table interface {
	GetName() string
}

// Dummy table with name
type FakeTable string

func (f FakeTable) GetName() string {
	return string(f)
}

type prefixedTable struct {
	prefix string
	t      Table
}

func (t prefixedTable) GetName() string {
	return fmt.Sprintf("`%s`", path.Join(t.prefix, t.t.GetName()))
}

// Kikimr database connection
type Database struct {
	config DatabaseConfig
	tables map[string]Table
}

func (db *Database) GetConfig() DatabaseConfig {
	return db.config
}

func (db *Database) newYdbTrace() tracing.YdbTrace {
	return tracing.NewYDBTrace(&tracing.YDBTraceConfig{
		Driver:      db.config.TraceDriver,
		Client:      db.config.TraceClient,
		SessionPool: db.config.TraceSessionPool,
	})
}

func (db *Database) newDialer(trace tracing.YdbTrace, credentials ydb.Credentials) ydb.Dialer {
	var tlsConfig *tls.Config
	if db.config.TLS {
		// use default TLS settings with trust to system with CA
		tlsConfig = &tls.Config{}
	}
	driverConfig := &ydb.DriverConfig{
		Database:             db.config.DBName,
		PreferLocalEndpoints: true,
		OperationTimeout:     time.Duration(db.config.OperationTimeoutMs) * time.Millisecond,
		Trace:                trace.Driver,
	}

	if credentials != nil {
		driverConfig.Credentials = credentials
	}

	return ydb.Dialer{
		DriverConfig: driverConfig,
		TLSConfig:    tlsConfig,
	}
}

// Return connection
func (db *Database) Open(ctx context.Context, credentials ydb.Credentials) (*sql.DB, error) {
	trace := db.newYdbTrace()

	opts := []ydbsql.ConnectorOption{
		ydbsql.WithEndpoint(db.config.DBHost),
		ydbsql.WithClientTrace(trace.Client),
		ydbsql.WithSessionPoolTrace(trace.Session),
		ydbsql.WithSessionPoolSizeLimit(db.config.MaxSessionCount),
	}

	ctxlog.G(ctx).Info("Database config", zap.Any("config", db.config))

	opts = append(opts, ydbsql.WithDialer(db.newDialer(trace, credentials)))

	d := sql.OpenDB(ydbsql.Connector(opts...))
	d.SetMaxOpenConns(db.config.MaxConns)
	return d, d.Ping()
}

func (db *Database) Dial(ctx context.Context, credentials ydb.Credentials) (*table.Client, error) {
	trace := db.newYdbTrace()
	dialer := db.newDialer(trace, credentials)
	driver, err := dialer.Dial(ctx, db.config.DBHost)
	if err != nil {
		return nil, err
	}

	client := &table.Client{
		Driver: driver,
		Trace:  trace.Client,
	}
	return client, err
}

// Add table to list of known tables
func (db *Database) AddTable(t Table) {
	db.tables[t.GetName()] = t
}

// Get table by short name, nil if not found.
func (db *Database) GetTable(name string) Table {
	if _, ok := db.tables[name]; !ok {
		return nil
	}

	return prefixedTable{prefix: db.config.Root, t: db.tables[name]}
}

// Manages multiple kikimr database connections
type DatabaseProvider struct {
	dbs map[string]*Database
}

// Initialize databases from config
func NewDatabaseProvider(config Config) (*DatabaseProvider, error) {
	dbs := map[string]*Database{}
	for name, conf := range config {
		dbs[name] = &Database{conf, map[string]Table{}}
	}

	return &DatabaseProvider{dbs}, nil
}

// Get database by short name, nil if not found.
func (p *DatabaseProvider) GetDatabase(name string) *Database {
	return p.dbs[name]
}

// Get table by slash-separated path (database/table), nil if not found.
func (p *DatabaseProvider) GetTable(path string) Table {
	parts := strings.Split(path, "/")
	if len(parts) != 2 {
		return nil
	}

	if p.GetDatabase(parts[0]) == nil {
		return nil
	}

	return p.GetDatabase(parts[0]).GetTable(parts[1])
}

// Check that all specified paths (database/table) are registered.
func (p *DatabaseProvider) Verify(paths ...string) error {
	for _, path := range paths {
		parts := strings.Split(path, "/")
		if len(parts) != 2 {
			return fmt.Errorf("invalid path: %s", path)
		}
		if p.GetDatabase(parts[0]) == nil {
			return fmt.Errorf("no database %s for path: %s", parts[0], path)
		}
		if p.GetDatabase(parts[0]).GetTable(parts[1]) == nil {
			return fmt.Errorf("no table %s for path: %s", parts[1], path)
		}
	}
	return nil
}

// Register paths (database/table).
func (p *DatabaseProvider) CreateFakeTables(paths ...string) error {
	for _, path := range paths {
		parts := strings.Split(path, "/")
		if len(parts) != 2 {
			return fmt.Errorf("invalid path: %s", path)
		}
		if p.GetDatabase(parts[0]) == nil {
			return fmt.Errorf("no database %s for path: %s", parts[0], path)
		}
		p.GetDatabase(parts[0]).AddTable(FakeTable(parts[1]))
	}
	return nil
}

var databaseConfig *DatabaseConfig

// Replace vagrant in root
func SubstituteVariables(config DatabaseConfig) (DatabaseConfig, error) {
	if config.DBHost == "" {
		return config, errors.New("kikimr endpoint is not specified in the configuration file")
	}
	if config.Root == "" {
		return config, errors.New("kikimr root is not specified in the configuration file")
	}

	// Default connection limit
	if config.MaxConns == 0 {
		config.MaxConns = defaultMaxConns
	}

	vagrantSubstitutionString := "/$vagrant/"

	if strings.Contains(config.Root, vagrantSubstitutionString) {
		currentUser, err := user.Current()
		if err != nil {
			return config, fmt.Errorf("unable to determine current user: %s", err)
		}

		if currentUser.Username != "vagrant" {
			return config, errors.New("unable to expand $vagrant variable in KiKiMR root path: we aren't running under Vagrant")
		}

		machineID, err := ioutil.ReadFile("/vagrant/.vagrant/machines/default/virtualbox/id")
		if err != nil {
			return config, fmt.Errorf("unable to determine Vagrant machine ID: %s", err)
		}

		config.Root = strings.Replace(
			config.Root, vagrantSubstitutionString, "/vagrant/"+strings.TrimSpace(string(machineID))+"/", -1)
	}

	if strings.Contains(config.Root, "$") {
		return config, errors.New("kikimr root path contains an unbound variable")
	}

	return config, nil
}
