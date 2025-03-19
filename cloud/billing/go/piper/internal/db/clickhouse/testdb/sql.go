package testdb

import (
	"context"
	"os"
	"path"
)

func CleanupSchema(ctx context.Context, scriptPath string) error {
	return runDMLFile(ctx, path.Join(scriptPath, "cleanup.sql"))
}

func SetupSchema(ctx context.Context, scriptPath string) {
	if err := runSchemaFile(ctx, path.Join(scriptPath, "setup.sql")); err != nil {
		panic(err)
	}
}

func TearDownSchema(ctx context.Context, scriptPath string) {
	if err := runSchemaFile(ctx, path.Join(scriptPath, "teardown.sql")); err != nil {
		panic(err)
	}
}

func runDMLFile(ctx context.Context, filename string) error {
	content, err := os.ReadFile(filename)
	if err != nil {
		return err
	}
	_, err = DB().ExecContext(ctx, string(content))
	return err
}

func runSchemaFile(ctx context.Context, filename string) error {
	content, err := os.ReadFile(filename)
	if err != nil {
		return err
	}
	_, err = DB().ExecContext(ctx, string(content))
	return err
}
