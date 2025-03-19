package dbteststeps

import (
	"fmt"
	"io/ioutil"
	"os"
	"path"
	"strconv"
	"strings"

	"github.com/DATA-DOG/godog/gherkin"
	"github.com/jmoiron/sqlx"
	"github.com/stretchr/testify/assert"
	"gopkg.in/yaml.v2"

	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/pgutil/pgerrors"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/test/yatest"
)

var (
	queryDropSchema = sqlutil.Stmt{
		Name:  "DropSchema",
		Query: "DROP SCHEMA IF EXISTS %s CASCADE",
	}
	queryDropDatabase = sqlutil.Stmt{
		Name:  "DropDatabase",
		Query: "DROP DATABASE IF EXISTS %s",
	}
	queryCreateDatabase = sqlutil.Stmt{
		Name:  "CreateDatabase",
		Query: "CREATE DATABASE %s",
	}
	queryCreatePGCheckExtension = sqlutil.Stmt{
		Name:  "CreatePGCheckExtension",
		Query: "CREATE EXTENSION IF NOT EXISTS plpgsql_check",
	}
	querySelectSchemaFuncs = sqlutil.Stmt{
		Name: "SelectSchemaFuncs",
		Query: `SELECT p.oid AS func_oid, p.proname AS func_name
FROM pg_catalog.pg_namespace n
JOIN pg_catalog.pg_proc p ON pronamespace = n.oid
JOIN pg_catalog.pg_language l ON p.prolang = l.oid
WHERE n.nspname = :schema
	AND l.lanname = 'plpgsql'
	AND p.prorettype <> 2279`,
	}
	queryCheckFunction = sqlutil.Stmt{
		Name:  "CheckFunction",
		Query: "SELECT * FROM plpgsql_check_function(CAST(:oid AS oid)) AS msg",
	}
)

func (dbst *DBSteps) initializedLinter() error {
	_, err := sqlutil.QueryNode(dbst.TC.Context(), dbst.Node, queryCreatePGCheckExtension, nil, sqlutil.NopParser, dbst.L)
	return err
}

type LintError struct {
	Func  string
	OID   string
	Error string
}

func (e *LintError) String() string {
	return fmt.Sprintf("Func: %q Lint: %q", e.Func, e.Error)
}

type LintErrors struct {
	Lint []*LintError
}

func (e *LintErrors) Error() string {
	var s []string
	for _, l := range e.Lint {
		s = append(s, l.String())
	}

	return strings.Join(s, "\n")
}

func (dbst *DBSteps) linterFindNothingForFunctionsInSchema(arg1 string) error {
	type Func struct {
		OID  string `db:"func_oid"`
		Name string `db:"func_name"`
	}

	var funcs []Func
	funcsParser := func(rows *sqlx.Rows) error {
		var f Func
		if err := rows.StructScan(&f); err != nil {
			return err
		}

		funcs = append(funcs, f)
		return nil
	}
	_, err := sqlutil.QueryNode(dbst.TC.Context(), dbst.Node, querySelectSchemaFuncs, map[string]interface{}{"schema": arg1}, funcsParser, dbst.L)
	if err != nil {
		return err
	}

	var lerr LintErrors
	for _, f := range funcs {
		var res string
		failParser := func(rows *sqlx.Rows) error {
			return rows.Scan(&res)
		}
		count, err := sqlutil.QueryNode(dbst.TC.Context(), dbst.Node, queryCheckFunction, map[string]interface{}{"oid": f.OID}, failParser, dbst.L)
		if err != nil {
			return err
		}

		if count == 0 {
			continue
		}

		lerr.Lint = append(lerr.Lint, &LintError{Func: f.Name, OID: f.OID, Error: res})
	}

	if len(lerr.Lint) != 0 {
		return &lerr
	}

	return nil
}

func (dbst *DBSteps) databaseAtLastMigration() error {
	return dbst.applyOneFileMigration(dbst.Params.DBName)
}

func (dbst *DBSteps) databaseFromFile(arg1 string) error {
	if err := dbst.recreateDatabase(arg1); err != nil {
		return err
	}

	return dbst.applyOneFileMigration(arg1)
}

func (dbst *DBSteps) applyOneFileMigration(dbname string) error {
	if _, err := sqlutil.QueryNode(dbst.TC.Context(), dbst.Node, queryDropSchema.Format(dbst.Params.DataSchema), nil, sqlutil.NopParser, dbst.L); err != nil {
		return xerrors.Errorf("failed to drop schema: %w", err)
	}

	root, err := dbst.prepareMigrationFromOneFile(dbst.Params.ArcadiaPath, dbst.Params.SingleFile)
	if err != nil {
		return err
	}

	dsn := DSN(dbname, dbst.Params.DBName)

	if err := runPGMigrate(
		root,
		dsn,
		[]string{"clean"},
	); err != nil {
		return xerrors.Errorf("failed to clean db: %w", err)
	}

	if err := runPGMigrate(
		root,
		dsn,
		[]string{"migrate", "-t", "latest"},
	); err != nil {
		return xerrors.Errorf("failed to migrate db: %w", err)
	}

	return dbst.InitClusterClient(dbname)
}

func (dbst *DBSteps) recreateDatabase(dbname string) error {
	if _, err := sqlutil.QueryNode(dbst.TC.Context(), dbst.Node, queryDropDatabase.Format(dbname), nil, sqlutil.NopParser, dbst.L); err != nil {
		return xerrors.Errorf("failed to drop database: %w", err)
	}
	if _, err := sqlutil.QueryNode(dbst.TC.Context(), dbst.Node, queryCreateDatabase.Format(dbname), nil, sqlutil.NopParser, dbst.L); err != nil {
		return xerrors.Errorf("failed to create database: %w", err)
	}

	return nil
}

type MigrationsConfig struct {
	Callbacks CallbacksConfig `yaml:"callbacks,omitempty"`
	Conn      string          `yaml:"conn,omitempty"`
	Target    string          `yaml:"target,omitempty"`
}

type CallbacksConfig struct {
	AfterAll []string `yaml:"afterAll,omitempty"`
}

func (dbst *DBSteps) prepareMigrationFromOneFile(source, fullDDL string) (string, error) {
	if source == "" {
		panic("Got empty source!")
	}
	copyRoot := yatest.WorkPath(path.Join("migration", source))
	if _, ok := dbst.Migrations[source]; ok {
		return copyRoot, nil
	}

	cfgPath := yatest.SourcePath(path.Join(source, "migrations.yml"))
	data, err := ioutil.ReadFile(cfgPath)
	if err != nil {
		return "", xerrors.Errorf("failed to load migrations config from %q: %w", cfgPath, err)
	}

	var cfg MigrationsConfig
	if err = yaml.Unmarshal(data, &cfg); err != nil {
		return "", xerrors.Errorf("failed to unmarshal migrations config: %w", err)
	}

	if err = os.MkdirAll(copyRoot, 0777); err != nil {
		return "", err
	}

	for _, hook := range cfg.Callbacks.AfterAll {
		from := yatest.SourcePath(path.Join(source, hook))
		to := path.Join(copyRoot, hook)
		// Create top dirs if there are any
		dir := path.Dir(to)
		if dir != "" {
			if err = os.MkdirAll(dir, 0777); err != nil && xerrors.Is(err, os.ErrExist) {
				return "", err
			}
		}
		if err = os.Symlink(from, to); err != nil {
			return "", err
		}
	}

	if err = os.Symlink(cfgPath, path.Join(copyRoot, "migrations.yml")); err != nil {
		return "", err
	}

	migrationsPath := path.Join(copyRoot, "migrations")
	if err = os.MkdirAll(migrationsPath, 0777); err != nil {
		return "", err
	}

	if err = os.Symlink(yatest.SourcePath(path.Join(source, fullDDL)), path.Join(migrationsPath, "V0001__"+fullDDL)); err != nil {
		return "", err
	}

	dbst.Migrations[source] = struct{}{}
	return copyRoot, nil
}

func (dbst *DBSteps) databaseFromMigrations(arg1 string) error {
	if err := dbst.recreateDatabase(arg1); err != nil {
		return err
	}

	if err := runPGMigrate(
		yatest.SourcePath(dbst.Params.ArcadiaPath),
		DSN(arg1, dbst.Params.DBName),
		[]string{"migrate", "-t", "latest"},
	); err != nil {
		return xerrors.Errorf("failed to migrate db: %w", err)
	}

	return dbst.InitClusterClient(arg1)
}

func (dbst *DBSteps) iDumpTo(arg1, arg2 string) error {
	out := yatest.WorkPath(dbst.Params.DBName + "_pgdump")
	if err := os.MkdirAll(out, 0777); err != nil {
		return err
	}

	return runPGDump(DBHost(dbst.Params.DBName), DBPort(dbst.Params.DBName), dbst.Params.DataSchema, arg1, path.Join(out, arg2))
}

func (dbst *DBSteps) thereAreNoDifferencesInAnd(arg1, arg2 string) error {
	out := yatest.WorkPath(dbst.Params.DBName + "_pgdump")
	return runDiff(path.Join(out, arg1), path.Join(out, arg2))
}

func (dbst *DBSteps) iExecuteQuery(arg1 *gherkin.DocString) error {
	dbst.L.Debugf("executing query: %s", arg1.Content)
	dbst.Result = QueryResult{}
	query := fmt.Sprintf("SELECT to_json(r) FROM (%s) r", arg1.Content)
	rows, err := dbst.Node.DBx().QueryxContext(dbst.TC.Context(), query)
	if err != nil {
		dbst.L.Warnf("error executing query %q: %s", query, err)
		dbst.Result.Error = err
		return nil
	}
	defer func() { _ = rows.Close() }()

	for rows.Next() {
		var data []byte
		if err = rows.Scan(&data); err != nil {
			dbst.L.Warnf("error performing scan on query %q: %s", query, err)
			dbst.Result.Error = err
			return nil
		}

		// We use yaml so that it behaves exactly as unmarshaling for data in feature files
		var res interface{}
		if err = yaml.Unmarshal(data, &res); err != nil {
			dbst.L.Warnf("error unmarshaling yaml data on query %q: %s", query, err)
			dbst.Result.Error = err
			return nil
		}

		dbst.Result.Res = append(dbst.Result.Res, res)
	}

	if rows.Err() != nil {
		dbst.L.Warnf("error iterating over rows on query %q: %s", query, rows.Err())
		dbst.Result.Error = rows.Err()
	}

	return nil
}

// iExecuteScript execute query but don't fetch its result
func (dbst *DBSteps) iExecuteScript(arg1 *gherkin.DocString) error {
	dbst.L.Debugf("executing script: %s", arg1.Content)
	dbst.Result = QueryResult{}
	query := arg1.Content
	res, err := dbst.Node.DBx().ExecContext(dbst.TC.Context(), query)
	if err != nil {
		dbst.L.Warnf("error executing script %q: %s", query, err)
		dbst.Result.Error = err
		return nil
	}
	_, err = res.RowsAffected()
	if err != nil {
		dbst.L.Warnf("error executing script %q: %s", query, err)
		dbst.Result.Error = err
		return nil
	}
	return nil
}

func (dbst *DBSteps) itReturnsOneRowMatches(arg1 *gherkin.DocString) error {
	if dbst.Result.Error != nil {
		return xerrors.Errorf("expected one result but received error: %w", dbst.Result.Error)
	}

	expected := make(map[interface{}]interface{})
	if err := yaml.Unmarshal([]byte(arg1.Content), &expected); err != nil {
		return xerrors.Errorf("failed to unmarshal %q: %w", arg1.Content, err)
	}

	if len(expected) == 0 {
		return xerrors.New("expected one result but was asked to verify nothing")
	}

	if len(dbst.Result.Res) != 1 {
		return xerrors.Errorf("expected one result but received %d", len(dbst.Result.Res))
	}

	return rowInRows(expected, dbst.Result.Res)
}

func (dbst *DBSteps) itReturnsRowsMatches(arg1 string, arg2 *gherkin.DocString) error {
	if dbst.Result.Error != nil {
		return xerrors.Errorf("expected result but received error: %w", dbst.Result.Error)
	}

	expectedCount, err := strconv.ParseInt(arg1, 10, 64)
	if err != nil {
		return xerrors.Errorf("couldn't parse rows count: %w", dbst.Result.Error)
	}

	var expected []map[interface{}]interface{}
	if err := yaml.Unmarshal([]byte(arg2.Content), &expected); err != nil {
		return xerrors.Errorf("failed to unmarshal %q: %w", arg2.Content, err)
	}

	if len(expected) != int(expectedCount) {
		return xerrors.Errorf("expected %d result but was asked to verify %d", expectedCount, len(expected))
	}

	if len(dbst.Result.Res) != int(expectedCount) {
		return xerrors.Errorf("expected %d result but received %d", expectedCount, len(dbst.Result.Res))
	}

	return rowsInRows(expected, dbst.Result.Res)
}

func rowsInRows(expected []map[interface{}]interface{}, actual []interface{}) error {
	for _, row := range expected {
		if err := rowInRows(row, actual); err != nil {
			return xerrors.Errorf("failed to find row: %+v; received %+v", row, actual)
		}
	}

	return nil
}

func rowInRows(expected map[interface{}]interface{}, actual []interface{}) error {
	var err error
	for _, row := range actual {
		if err = rowInRow(expected, row); err == nil {
			return nil
		}
	}

	return err
}

func rowInRow(expected map[interface{}]interface{}, actual interface{}) error {
	m, ok := actual.(map[interface{}]interface{})
	if !ok {
		return xerrors.Errorf("invalid actual type %T: %+v", actual, actual)
	}

	for k, v := range expected {
		actualValue, ok := m[k]
		if !ok {
			return xerrors.Errorf("key %q is missing from result: received %+v", k, m)
		}

		if !assert.ObjectsAreEqualValues(v, actualValue) {
			return xerrors.Errorf("key %q is expected to have %s of type %T value but has %s of type %T: received %+v", k, v, v, actualValue, actualValue, m)
		}
	}

	return nil
}

func (dbst *DBSteps) iSuccessfullyExecuteQuery(arg1 *gherkin.DocString) error {
	dbst.Result = QueryResult{}
	dbst.L.Debugf("executing query: %s", arg1.Content)
	_, err := dbst.Node.DBx().ExecContext(dbst.TC.Context(), arg1.Content)
	return err
}

func (dbst *DBSteps) itFailWithError(text string) error {
	if dbst.Result.Error == nil {
		return xerrors.New("expected an error but received none")
	}

	if !strings.Contains(dbst.Result.Error.Error(), text) {
		return xerrors.Errorf("expected error %q but received %q", text, dbst.Result.Error)
	}

	return nil
}

func (dbst *DBSteps) itFailWithErrorAndCode(text, code string) error {
	if err := dbst.itFailWithError(text); err != nil {
		return err
	}

	c, ok := pgerrors.Code(dbst.Result.Error)
	if !ok {
		return xerrors.Errorf("expected pgx error but it isn't: %w", dbst.Result.Error)
	}

	if c != code {
		return xerrors.Errorf("expected error code %q but received %q", code, c)
	}

	return nil
}

func (dbst *DBSteps) itReturnsNothing() error {
	if dbst.Result.Error != nil {
		return xerrors.Errorf("expected no result but received error: %w", dbst.Result.Error)
	}

	if len(dbst.Result.Res) != 0 {
		return xerrors.Errorf("expected no result but received %d", len(dbst.Result.Res))
	}

	return nil
}

func (dbst *DBSteps) itSuccess() error {
	return dbst.Result.Error
}
