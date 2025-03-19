package migration

import (
	"context"
	"fmt"

	"a.yandex-team.ru/cloud/marketplace/license_server/internal/app"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/app/config"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/db/ydb/migrations"
	"a.yandex-team.ru/cloud/marketplace/pkg/auth"
	"a.yandex-team.ru/cloud/marketplace/pkg/logging"
	"a.yandex-team.ru/cloud/marketplace/pkg/ydb"
)

type authContext struct {
	ctx    context.Context
	cancel context.CancelFunc
}

func migrateBase(configProxyPaths []string, defaultTokenAuth *auth.YCDefaultTokenAuthenticator) (*migrations.Migrator, error) {
	cfg, err := config.LoadMigrationConfig(context.Background(), configProxyPaths)
	if err != nil {
		fmt.Println("failed load config", err)
		return nil, err
	}

	if err := app.SetupLogging(cfg.Logger); err != nil {
		return nil, err
	}

	ydbOptions, err := app.MakeYDBConfig(cfg.YDB)
	if err != nil {
		return nil, err
	}

	logger := logging.Named("migration")

	ydbOptions = append(ydbOptions,
		ydb.WithLogger(logger.WithName("ydb")),
		ydb.WithYDBCredentials(defaultTokenAuth),
	)

	ydbConnection, err := ydb.Connect(context.Background(), ydbOptions...)
	if err != nil {
		return nil, err
	}

	m := migrations.NewMigrator(ydbConnection)
	return m, nil
}

func MigrateInitUp(configProxyPaths []string) error {
	ctx := context.Background()

	authCtx := &authContext{}

	authCtx.ctx, authCtx.cancel = context.WithCancel(ctx)
	defer authCtx.cancel()
	defaultTokenAuth := auth.NewYCDefaultTokenAuthenticator(authCtx.ctx)

	m, err := migrateBase(configProxyPaths, defaultTokenAuth)
	if err != nil {
		return err
	}

	return m.InitUp(ctx)
}

func MigrateInitDown(configProxyPaths []string) error {
	ctx := context.Background()

	authCtx := &authContext{}

	authCtx.ctx, authCtx.cancel = context.WithCancel(ctx)
	defer authCtx.cancel()
	defaultTokenAuth := auth.NewYCDefaultTokenAuthenticator(authCtx.ctx)

	m, err := migrateBase(configProxyPaths, defaultTokenAuth)
	if err != nil {
		return err
	}

	return m.InitDown(ctx)
}

func MigrateUp(configProxyPaths []string) error {
	ctx := context.Background()

	authCtx := &authContext{}

	authCtx.ctx, authCtx.cancel = context.WithCancel(ctx)
	defer authCtx.cancel()
	defaultTokenAuth := auth.NewYCDefaultTokenAuthenticator(authCtx.ctx)

	m, err := migrateBase(configProxyPaths, defaultTokenAuth)
	if err != nil {
		return err
	}

	return m.Up(ctx)
}

func MigrateDown(configProxyPaths []string) error {
	ctx := context.Background()

	authCtx := &authContext{}

	authCtx.ctx, authCtx.cancel = context.WithCancel(ctx)
	defer authCtx.cancel()
	defaultTokenAuth := auth.NewYCDefaultTokenAuthenticator(authCtx.ctx)

	m, err := migrateBase(configProxyPaths, defaultTokenAuth)
	if err != nil {
		return err
	}

	return m.Down(ctx)
}
