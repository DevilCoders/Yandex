package testdb

import (
	"context"
	"os"
	"path"

	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/table"
)

func SetupSchema(ctx context.Context, scriptPath string) {
	if err := runSchemaFile(ctx, path.Join(scriptPath, "setup.sql")); err != nil {
		panic(err)
	}
}

func CleanupSchema(ctx context.Context, scriptPath string) error {
	return runDMLFile(ctx, path.Join(scriptPath, "cleanup.sql"))
}

func TearDownSchema(ctx context.Context, scriptPath string) {
	if err := runSchemaFile(ctx, path.Join(scriptPath, "teardown.sql")); err != nil {
		panic(err)
	}
}

func runSchemaFile(ctx context.Context, filename string) error {
	sess, err := YDBPool().Get(ctx)
	if err != nil {
		return err
	}
	content, err := os.ReadFile(filename)
	if err != nil {
		return err
	}
	return sess.ExecuteSchemeQuery(ctx, string(content))
}

func runDMLFile(ctx context.Context, filename string) error {
	sess, err := YDBPool().Get(ctx)
	if err != nil {
		return err
	}
	content, err := os.ReadFile(filename)
	if err != nil {
		return err
	}
	_, _, err = sess.Execute(ctx, table.TxControl(
		table.BeginTx(
			table.WithSerializableReadWrite(),
		),
		table.CommitTx(),
	), string(content), nil)
	return err
}
