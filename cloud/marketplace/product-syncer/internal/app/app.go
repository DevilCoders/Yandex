package app

import (
	"context"
	"fmt"
	"io"

	"golang.org/x/sync/errgroup"

	"a.yandex-team.ru/cloud/marketplace/pkg/auth"
	"a.yandex-team.ru/cloud/marketplace/pkg/clients/compute"
	"a.yandex-team.ru/cloud/marketplace/pkg/clients/marketplace-private"
	"a.yandex-team.ru/cloud/marketplace/pkg/clients/operation"
	"a.yandex-team.ru/cloud/marketplace/pkg/ctxtools"
	"a.yandex-team.ru/cloud/marketplace/pkg/logging"
	monitoring "a.yandex-team.ru/cloud/marketplace/pkg/monitoring/http"
	"a.yandex-team.ru/cloud/marketplace/pkg/tracing"
	"a.yandex-team.ru/cloud/marketplace/product-syncer/internal/app/config"
	"a.yandex-team.ru/cloud/marketplace/product-syncer/internal/services/env"
	sync_service "a.yandex-team.ru/cloud/marketplace/product-syncer/internal/services/sync"
	"a.yandex-team.ru/library/go/core/log"
)

const (
	serviceName = "mkt.product-syncer"
)

type Application struct {
	monitoringService *monitoring.Service

	tracerCloser io.Closer

	syncService *sync_service.Service

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

	if err := setupLogging(cfg.Logger); err != nil {
		return nil, err
	}

	logger := logging.Named("app")

	logger.Info("starting service initialization...")

	logger.Info("initialization of tracer...")

	tracer, tracerCloser, err := tracing.NewTracer(
		makeTracingConfig(serviceName, cfg.Tracer),
	)

	if err != nil {
		return nil, err
	}

	tracing.SetTracer(tracer)
	app.tracerCloser = tracerCloser
	logger.Info("initialization of tracer has been completed")

	app.authCtx.ctx, app.authCtx.cancel = context.WithCancel(context.Background())
	defaultTokenAuth := auth.NewYCDefaultTokenAuthenticator(app.authCtx.ctx)

	logger.Info("initialization of marketplace client...")
	marketplaceConfig, err := makeMarketplaceClientConfig(cfg.MarketplaceClient)
	if err != nil {
		return nil, err
	}

	marketplaceClient := marketplace.NewClient(
		marketplaceConfig,
		logger.WithName("marketplace-private"),
	)
	logger.Info("initialization of marketplace client has been completed")

	logger.Info("initialization of compute image client...")
	computeConfig := makeComputeClientConfig(cfg.YcClient)
	if computeConfig == nil {
		return nil, fmt.Errorf("marketplace client config should be provided")
	}

	computeClient, err := compute.NewClientWithAuthProvider(
		*computeConfig,
		defaultTokenAuth,
		logger.WithName("compute-client"),
	)
	if err != nil {
		return nil, err
	}
	logger.Info("initialization of compute image client has been completed")

	logger.Info("initialization of cloud operations client...")
	operationConfig, err := makeOperationConfig(cfg.YcClient)
	if err != nil {
		return nil, err
	}

	operationClient, err := operation.NewClientWithAuthProvider(
		*operationConfig,
		defaultTokenAuth,
		logger.WithName("operation-client"),
	)
	if err != nil {
		return nil, err
	}
	logger.Info("initialization of cloud operations client has been completed")

	logger.Info("constructing context environment...")
	envBuilder := env.NewEnvBuilder()
	envBuilder.WithBackendsFabrics(
		env.BackendsWithMarketplace(marketplaceClient),
		//env.BackendsWithResourceManager(resourceManager),
		env.BackendsWithComputeImage(computeClient),
		env.BackendsWithOperation(operationClient),
		env.BackendsWithYCDefaultCredentials(defaultTokenAuth),
	)

	env := envBuilder.
		WithHandlersLogger(logging.Named("handlers")).
		//WithAuthBackend(authBackend).
		Build()

	logger.Info("initialization of sync backend...")
	app.syncService, err = sync_service.NewService(env, cfg)
	if err != nil {
		return nil, err
	}

	logger.Info("initialization of sync backend has been completed")

	logger.Info("constructing of context environment has been completed")
	logger.Info("service initialization completed")
	return app, nil
}

func (a *Application) Run(runCtx context.Context) error {
	a.DefaultLogger().Debug("running application")

	defer a.DefaultLogger().Debug("stopping application")
	//defer a.authCtx.cancel()
	defer func() {
		if a.tracerCloser == nil {
			return
		}

		if err := a.tracerCloser.Close(); err != nil {
			a.DefaultLogger().Error("failed to close tracer", log.Error(err))
		}
	}()

	group, ctx := errgroup.WithContext(runCtx)
	ctx = ctxtools.WithStructuredLogger(ctx, a.DefaultLogger().Structured())

	if a.monitoringService != nil {
		group.Go(func() error {
			a.DefaultLogger().Info("running monitoring service")
			return a.monitoringService.Run(ctx)
		})
	}

	group.Go(func() error {
		a.DefaultLogger().Info("running sync service")
		return a.syncService.Run(ctx)
	})

	return group.Wait()
}

func (a *Application) DefaultLogger() log.Logger {
	return logging.Logger().Logger()
}
