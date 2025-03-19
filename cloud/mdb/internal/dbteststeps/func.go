package dbteststeps

import (
	"context"
	"fmt"
	"os"
	"strings"
	"time"

	"github.com/DATA-DOG/godog"
	"github.com/go-cmd/cmd"

	"a.yandex-team.ru/cloud/mdb/internal/godogutil"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/pgutil"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/test/yatest"
)

type Params struct {
	DBName      string
	SingleFile  string
	ArcadiaPath string
	DataSchema  string
}

type DBSteps struct {
	L log.Logger

	// Migrations lists prepared single-file migrations so test does not have to reprepare them
	Migrations map[string]struct{}

	Cluster *sqlutil.Cluster
	Node    sqlutil.Node

	Result QueryResult

	TC *godogutil.TestContext

	Params Params
}

func RegisterSteps(dbst *DBSteps, s *godog.Suite) {
	s.Step(`^database at last migration$`, dbst.databaseAtLastMigration)
	s.Step(`^initialized linter$`, dbst.initializedLinter)
	s.Step(`^linter find nothing for functions in schema "([^"]*)"$`, dbst.linterFindNothingForFunctionsInSchema)
	s.Step(`^"([^"]*)" database from `+dbst.Params.SingleFile+"$", dbst.databaseFromFile)
	s.Step(`^"([^"]*)" database from migrations\/$`, dbst.databaseFromMigrations)
	s.Step(`^I dump "([^"]*)" to "([^"]*)"$`, dbst.iDumpTo)
	s.Step(`^there are no differences in "([^"]*)" and "([^"]*)"$`, dbst.thereAreNoDifferencesInAnd)
	s.Step(`^I execute query$`, dbst.iExecuteQuery)
	s.Step(`^I execute script$`, dbst.iExecuteScript)
	s.Step(`^it returns one row matches$`, dbst.itReturnsOneRowMatches)
	s.Step(`^it returns "([^"]*)" rows matches$`, dbst.itReturnsRowsMatches)
	// This must be before next line (or next line will always match)
	s.Step(`^it fail with error "(.+)" and code "(.+)"$`, dbst.itFailWithErrorAndCode)
	s.Step(`^it fail with error "(.+)"$`, dbst.itFailWithError)
	s.Step(`^I successfully execute query$`, dbst.iSuccessfullyExecuteQuery)
	s.Step(`^successfully executed query$`, dbst.iSuccessfullyExecuteQuery)
	s.Step(`^it returns nothing$`, dbst.itReturnsNothing)
	s.Step(`^it success$`, dbst.itSuccess)
}

func RegisterStepsWithDBName(dbst *DBSteps, s *godog.Suite, cluster string) {
	s.Step(`^database at last migration in `+cluster, dbst.databaseAtLastMigration)
	s.Step(`^initialized linter for `+cluster, dbst.initializedLinter)
	s.Step(`^linter find nothing for functions in schema "([^"]*)" in `+cluster, dbst.linterFindNothingForFunctionsInSchema)
	s.Step(`^"([^"]*)" database from `+dbst.Params.SingleFile+` in `+cluster, dbst.databaseFromFile)
	s.Step(`^"([^"]*)" database from migrations in `+cluster, dbst.databaseFromMigrations)
	s.Step(`^I dump "([^"]*)" to "([^"]*)" in `+cluster, dbst.iDumpTo)
	s.Step(`^there are no differences in "([^"]*)" and "([^"]*)" in `+cluster, dbst.thereAreNoDifferencesInAnd)
	s.Step(`^I execute query in `+cluster, dbst.iExecuteQuery)
	s.Step(`^I execute script in `+cluster, dbst.iExecuteScript)
	s.Step(`^it returns from `+cluster+` one row matches$`, dbst.itReturnsOneRowMatches)
	s.Step(`^it returns from `+cluster+` "([^"]*)" rows matches$`, dbst.itReturnsRowsMatches)
	// This must be before next line (or next line will always match)
	s.Step(`^it fail in `+cluster+` with error "(.+)" and code "(.+)"$`, dbst.itFailWithErrorAndCode)
	s.Step(`^it fail in `+cluster+` with error "(.+)"$`, dbst.itFailWithError)
	s.Step(`^I successfully execute query in `+cluster, dbst.iSuccessfullyExecuteQuery)
	s.Step(`^successfully executed query in `+cluster, dbst.iSuccessfullyExecuteQuery)
	s.Step(`^it returns nothing from `+cluster, dbst.itReturnsNothing)
	s.Step(`^it success in cluster `+cluster, dbst.itSuccess)
}

type QueryResult struct {
	Res   []interface{}
	Error error
}

func DBHost(dbname string) string {
	varName := strings.ToUpper(dbname) + "_POSTGRESQL_RECIPE_HOST"
	host, ok := os.LookupEnv(varName)
	if !ok {
		panic(varName + " is missing")
	}

	return host
}

func DBPort(dbname string) string {
	varName := strings.ToUpper(dbname) + "_POSTGRESQL_RECIPE_PORT"
	port, ok := os.LookupEnv(varName)
	if !ok {
		panic(varName + "is missing")
	}

	return port
}

func DBHostPort(dbname string) string {
	return fmt.Sprintf("%s:%s", DBHost(dbname), DBPort(dbname))
}

// New create new DBSteps
func New(tc *godogutil.TestContext, params Params) (*DBSteps, error) {
	l, err := zap.New(zap.KVConfig(log.DebugLevel))
	if err != nil {
		return nil, err
	}

	dbst := &DBSteps{
		L:          l,
		Migrations: make(map[string]struct{}),
		TC:         tc,
		Params:     params,
	}

	if err = dbst.InitClusterClient(params.DBName); err != nil {
		return nil, err
	}

	return dbst, nil
}

// NewReadyCluster create cluster and wait until it ready
func NewReadyCluster(clusterName, dbname string, opts ...sqlutil.ClusterOption) (*sqlutil.Cluster, sqlutil.Node, error) {
	db, err := pgutil.NewCluster(
		pgutil.Config{
			Addrs:   []string{DBHostPort(clusterName)},
			DB:      dbname,
			SSLMode: pgutil.AllowSSLMode,
		},
		opts...,
	)
	if err != nil {
		return nil, nil, xerrors.Errorf("failed to create cluster: %w", err)
	}

	ctx, cancel := context.WithTimeout(context.Background(), time.Second*10)
	defer cancel()
	master, err := waitForDB(ctx, db)
	if err != nil {
		return nil, nil, xerrors.Errorf("failed to wait for master of %q: %w", dbname, err)
	}
	return db, master, nil
}

// InitClusterClient closes old and creates new client to db. This is required at least after each 'drop schema'
// because OIDs are cached and will be invalid after schema recreation.
func (dbst *DBSteps) InitClusterClient(dbname string) error {
	if dbst.Cluster != nil {
		// Remove old cluster so that we won't reuse it accidentally
		old := dbst.Cluster
		dbst.Cluster = nil
		if err := old.Close(); err != nil {
			return xerrors.Errorf("failed to close cluster: %w", err)
		}
	}

	db, node, err := NewReadyCluster(dbst.Params.DBName, dbname)
	if err != nil {
		return xerrors.Errorf("failed to create cluster: %w", err)
	}
	dbst.Cluster = db
	dbst.Node = node
	return nil
}

func waitForDB(ctx context.Context, db *sqlutil.Cluster) (sqlutil.Node, error) {
	return db.WaitForPrimary(ctx)
}

func DSN(dbname, recipeName string) string {
	return pgutil.ConnString(
		DBHostPort(recipeName),
		dbname,
		"",
		"",
		pgutil.AllowSSLMode,
		"",
	)
}

func runPGMigrate(cwd, connstr string, args []string) error {
	c := cmd.NewCmd(yatest.BuildPath("contrib/python/yandex-pgmigrate/bin/pgmigrate"), append(args, "-vvv", "-c", connstr)...)
	c.Dir = cwd
	status := <-c.Start()

	if status.Error != nil {
		return xerrors.Errorf("pgmigrate failed unexpectedly: %w", status.Error)
	}

	if status.Exit != 0 {
		return xerrors.Errorf("pgmigrate failed while running, code %d, stdout %q, stderr %q", status.Exit, status.Stdout, status.Stderr)
	}

	return nil
}

func runPGDump(host, port, schema, dbname, to string) error {
	c := cmd.NewCmd(yatest.WorkPath("pg/bin/pg_dump"), "--host="+host, "--port="+port, "--schema="+schema, "--schema-only", "--no-owner", dbname, "-f", to)
	lib := yatest.WorkPath("pg/lib")
	env := os.Environ()

	prependPathVar := func(varName, addValue string) {
		if _, ok := os.LookupEnv(varName); ok {
			for i, old := range env {
				if !strings.HasPrefix(old, varName+"=") {
					continue
				}
				newValue := addValue + ":" + old[strings.Index(old, "=")+1:]
				env[i] = varName + "=" + newValue
			}
		} else {
			env = append(env, varName+"="+addValue)
		}
	}
	// prepend instead of append cause we don't want conflicts with user libraries
	prependPathVar("LD_LIBRARY_PATH", lib)
	// tiny workaround for OSX 'system' Postgre
	// > pg_dump: server version: 11.6; pg_dump version: 11.6
	prependPathVar("DYLD_FALLBACK_LIBRARY_PATH", lib)

	c.Env = env
	status := <-c.Start()

	if status.Error != nil {
		return xerrors.Errorf("pg_dump failed unexpectedly: %w", status.Error)
	}

	if status.Exit != 0 {
		return xerrors.Errorf("pg_dump failed while running, code %d, stdout %q, stderr %q", status.Exit, status.Stdout, status.Stderr)
	}

	return nil
}

func runDiff(one, two string) error {
	c := cmd.NewCmd("diff", one, two)
	status := <-c.Start()

	if status.Error != nil {
		return xerrors.Errorf("diff failed unexpectedly: %w", status.Error)
	}

	if status.Exit != 0 {
		return xerrors.Errorf("diff failed while running, code %d, stdout %q, stderr %q", status.Exit, status.Stdout, status.Stderr)
	}

	return nil
}
