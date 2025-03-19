package dbteststeps

import (
	"bytes"
	"context"
	"fmt"
	"io/ioutil"
	"os"
	"path"
	"time"

	"github.com/DATA-DOG/godog"
	"github.com/jackc/pgx/v4/stdlib"
	"github.com/jmoiron/sqlx"

	"a.yandex-team.ru/cloud/mdb/internal/godogutil"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/library/go/core/log/nop"
	"a.yandex-team.ru/library/go/core/xerrors"
)

var (
	querySelectSchemaTables = sqlutil.Stmt{
		Name: "SelectSchemaTables",
		// language=PostgreSQL
		Query: "SELECT table_name FROM information_schema.tables WHERE table_schema = :schema",
	}
)

func tablesInSchema(ctx context.Context, node sqlutil.Node, schema string) ([]string, error) {
	var tables []string

	parser := func(rows *sqlx.Rows) error {
		var name string
		if err := rows.Scan(&name); err != nil {
			return err
		}
		tables = append(tables, name)
		return nil
	}

	if _, err := sqlutil.QueryNode(
		ctx,
		node,
		querySelectSchemaTables,
		map[string]interface{}{"schema": schema},
		parser,
		&nop.Logger{},
	); err != nil {
		return nil, xerrors.Errorf("fail to get tables names: %w", err)
	}

	return tables, nil
}

func copyTable(ctx context.Context, node sqlutil.Node, tableFullName string) ([]byte, error) {
	db := node.DB()
	if db == nil {
		return nil, semerr.Unavailable("unavailable")
	}
	// COPY is Postgre specific. There are no COPY API support in database/sql level.
	// That is why we operate with pgx.Conn here
	pgConn, err := stdlib.AcquireConn(db)
	if err != nil {
		return nil, xerrors.Errorf("fail to acquire conn: %w", err)
	}
	defer func() { _ = stdlib.ReleaseConn(db, pgConn) }()

	retBuf := &bytes.Buffer{}
	copyQuery := fmt.Sprintf("COPY %s TO STDOUT WITH CSV HEADER", tableFullName)
	_, err = pgConn.PgConn().CopyTo(ctx, retBuf, copyQuery)

	if err != nil {
		return nil, xerrors.Errorf("COPY fail (%s): %w", copyQuery, err)
	}

	return retBuf.Bytes(), nil
}

// DumpSchema dump each table from schema to separate file in dir
func DumpSchema(ctx context.Context, node sqlutil.Node, schema, dir string) error {
	tables, err := tablesInSchema(ctx, node, schema)
	if err != nil {
		return err
	}
	for _, name := range tables {
		tableFullName := schema + "." + name

		csv, err := copyTable(ctx, node, tableFullName)
		if err != nil {
			return err
		}

		filePath := path.Join(dir, tableFullName+".csv")
		err = ioutil.WriteFile(filePath, csv, 0644)
		if err != nil {
			return xerrors.Errorf("fail to save %q csv data: %w", tableFullName, err)
		}
	}
	return nil
}

// DumpOnErrorHook return hook that can be used as godog.AfterScenario hook
// That dumps node schema with data
func DumpOnErrorHook(TC *godogutil.TestContext, node sqlutil.Node, toDir, schema string) func(interface{}, error) {
	dumpToPath := func() string {
		names := TC.LastExecutedNames()
		dumpDirPath := path.Join(toDir, names.FeatureName+" "+names.ScenarioName)
		return godogutil.TestOutputPath(dumpDirPath)
	}

	return func(_ interface{}, err error) {
		if err == nil || xerrors.Is(err, godog.ErrPending) {
			return
		}
		dumpDirPath := dumpToPath()
		if err := os.MkdirAll(dumpDirPath, 0755); err != nil {
			fmt.Printf("fail create dir: %q for database dump: %s\n", dumpDirPath, err)
			return
		}
		ctx, cancel := context.WithTimeout(context.Background(), time.Minute)
		defer cancel()
		dumpErr := DumpSchema(ctx, node, schema, dumpDirPath)
		if dumpErr != nil {
			fmt.Println(dumpErr)
		}
	}
}
