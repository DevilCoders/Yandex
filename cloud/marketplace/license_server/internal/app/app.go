package app

import (
	"context"
	"fmt"

	"golang.org/x/sync/errgroup"

	"a.yandex-team.ru/cloud/marketplace/license_server/internal/app/config"
	ydb_internal "a.yandex-team.ru/cloud/marketplace/license_server/internal/db/ydb"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/services/env"
	grpc_service "a.yandex-team.ru/cloud/marketplace/license_server/internal/services/grpc"
	"a.yandex-team.ru/cloud/marketplace/pkg/auth"
	"a.yandex-team.ru/cloud/marketplace/pkg/clients/billing-private"
	"a.yandex-team.ru/cloud/marketplace/pkg/clients/marketplace-private"
	"a.yandex-team.ru/cloud/marketplace/pkg/ctxtools"
	"a.yandex-team.ru/cloud/marketplace/pkg/logging"
	"a.yandex-team.ru/cloud/marketplace/pkg/ydb"
	"a.yandex-team.ru/library/go/core/log"
)

const (
	serviceName = "mkt.license_server"
)

type Application struct {
	grpcService *grpc_service.Service

	authCtx struct {
		ctx    context.Context
		cancel context.CancelFunc
	}
}

func NewApplication(initCtx context.Context, configProxyPaths []string) (*Application, error) {
	cfg, err := config.Load(initCtx, configProxyPaths)
	if err != nil {
		fmt.Println("failed load config", err)
		return nil, err
	}

	app := &Application{}

	if err := SetupLogging(cfg.Logger); err != nil {
		return nil, err
	}

	logger := logging.Named("app")

	logger.Info("started service initialization")

	logger.Info("initialization billing client...")

	billingConfig := makeBillingClientConfig(cfg.BillingClient)
	if billingConfig == nil {
		return nil, fmt.Errorf("billing client config should be provided")
	}

	billingClient := billing.NewClient(
		*billingConfig,
		logger.WithName("billing-private"),
	)

	logger.Info("initialization of billing client has been completed")

	logger.Info("initialization marketplace client...")

	marketplaceConfig := makeMarketplaceClientConfig(cfg.MarketplaceClient)
	if marketplaceConfig == nil {
		return nil, fmt.Errorf("marketplace client config should be provided")
	}

	marketplaceClient := marketplace.NewClient(
		*marketplaceConfig,
		logger.WithName("marketplace-private"),
	)

	logger.Info("initialization of marketplace client has been completed")

	logger.Info("initialization of authentication")

	app.authCtx.ctx, app.authCtx.cancel = context.WithCancel(context.Background())
	defaultTokenAuth := auth.NewYCDefaultTokenAuthenticator(app.authCtx.ctx)

	logger.Info("initialization of ydb connector...")

	ydbOptions, err := MakeYDBConfig(cfg.YDB)

	if err != nil {
		return nil, err
	}

	ydbOptions = append(ydbOptions,
		ydb.WithLogger(logger.WithName("ydb")),
		ydb.WithYDBCredentials(defaultTokenAuth),
	)

	ydbConnection, err := ydb.Connect(initCtx, ydbOptions...)
	if err != nil {
		return nil, err
	}

	logger.Info("initialization of ydb connector has been completed")

	logger.Info("constructing context environment...")

	envBuilder := env.NewEnvBuilder()
	envBuilder.WithBackendsFabrics(
		env.BackendsWithBilling(billingClient),
		env.BackendsWithMarketplace(marketplaceClient),
		env.BackendsWithYCDefaultCredentials(defaultTokenAuth),
		env.BackendsWithYdb(
			ydb_internal.NewLicenseServerProvider(ydbConnection),
		),
	)

	env := envBuilder.
		WithHandlersLogger(logging.Named("handlers")).
		WithCloudIDGeneratorPrefix(cfg.Prefix).
		Build()

	logger.Info("constructing of context environment has been completed")

	grpcOptions := makeGRPCServiceConfig(cfg.GRPC)
	if grpcOptions != nil {
		logger.Info("initialization of grpc backend...")
		app.grpcService, err = grpc_service.NewService(env, grpcOptions...)
		if err != nil {
			return nil, err
		}
		logger.Info("initialization of grpc backend has been completed")
	}

	logger.Info("service initialization completed")
	return app, nil
}

func (a *Application) Run(runCtx context.Context) error {
	a.DefaultLogger().Debug("running application")
	defer a.DefaultLogger().Debug("stopping application")
	defer a.authCtx.cancel()

	group, ctx := errgroup.WithContext(runCtx)
	ctx = ctxtools.WithStructuredLogger(ctx, a.DefaultLogger().Structured())

	group.Go(func() error {
		a.DefaultLogger().Info("running grpc service")
		return a.grpcService.Run(ctx)
	})

	return group.Wait()
}

func (a *Application) DefaultLogger() log.Logger {
	return logging.Logger().Logger()
}
