package app

import (
	"context"
	"fmt"
	"io"

	"golang.org/x/sync/errgroup"

	"a.yandex-team.ru/library/go/core/log"

	"a.yandex-team.ru/cloud/marketplace/pkg/auth"
	"a.yandex-team.ru/cloud/marketplace/pkg/clients/billing-private"
	"a.yandex-team.ru/cloud/marketplace/pkg/ctxtools"
	"a.yandex-team.ru/cloud/marketplace/pkg/logging"
	"a.yandex-team.ru/cloud/marketplace/pkg/tracing"
	"a.yandex-team.ru/cloud/marketplace/pkg/ydb"

	"a.yandex-team.ru/cloud/marketplace/lich/internal/app/config"
	"a.yandex-team.ru/cloud/marketplace/lich/internal/metrics"
	"a.yandex-team.ru/cloud/marketplace/lich/internal/services/env"

	grpc_service "a.yandex-team.ru/cloud/marketplace/lich/internal/services/grpc"
	http_service "a.yandex-team.ru/cloud/marketplace/lich/internal/services/http"

	ydb_internal "a.yandex-team.ru/cloud/marketplace/lich/internal/db/ydb"

	as "a.yandex-team.ru/cloud/marketplace/pkg/auth/access-backend"
	rm "a.yandex-team.ru/cloud/marketplace/pkg/clients/resource-manager"

	monitoring "a.yandex-team.ru/cloud/marketplace/pkg/monitoring/http"
)

const (
	serviceName = "mkt.license-check"
)

type Application struct {
	httpService *http_service.Service
	grpcService *grpc_service.Service

	monitoringService *monitoring.Service

	tracerCloser io.Closer

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

	logger.Info("initialization of resource manager...")

	app.authCtx.ctx, app.authCtx.cancel = context.WithCancel(context.Background())
	defaultTokenAuth := auth.NewYCDefaultTokenAuthenticator(app.authCtx.ctx)

	resourceManagerConfig := makeResourceManagerConfig(cfg.ResourceManager)
	if resourceManagerConfig == nil {
		return nil, fmt.Errorf("resource manager config required")
	}

	resourceManager, err := rm.NewClientWithAuthProvider(*resourceManagerConfig, defaultTokenAuth, logger.WithName("rm"))
	if err != nil {
		return nil, err
	}

	logger.Info("initialization of resource manager client has been completed")

	logger.Info("initialization of ydb connector...")

	ydbOptions, err := makeYDBConfig(cfg.YDB)
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

	logger.Info("initialization of auth backend...")

	authBackendConfig := makeAccessServiceConfig(cfg.AccessService)
	if authBackendConfig == nil {
		return nil, fmt.Errorf("configuration for access service should be provided")
	}

	authBackend, err := as.NewClient(initCtx, *authBackendConfig)
	if err != nil {
		return nil, err
	}

	logger.Info("initialization of auth backend has been completed")

	logger.Info("constructing context environment...")

	envBuilder := env.NewEnvBuilder()
	envBuilder.WithBackendsFabrics(
		env.BackendsWithBilling(billingClient),
		env.BackendsWithResourceManager(resourceManager),
		env.BackendsWithYCDefaultCredentials(defaultTokenAuth),
		env.BackendsWithYdb(
			ydb_internal.NewProductVersionsProvider(ydbConnection),
		),
	)

	metricsRegistry := metrics.NewHub()

	env := envBuilder.
		WithHandlersLogger(logging.Named("handlers")).
		WithAuthBackend(authBackend).
		WithMetricsHub(metricsRegistry).
		Build()

	logger.Info("constructing of context environment has been completed")

	if c := makeMonitoringConfig(cfg.Monitoring); c != nil {
		logger.Info("initialization of monitoring service...")
		app.monitoringService = monitoring.NewService(*c, metricsRegistry.Registry(), env.Backends().YDB())
		logger.Info("initialization of monitoring service has been completed")
	}

	httpConfig := makeHTTPServiceConfig(cfg.HTTP)
	if httpConfig != nil {
		logger.Info("initialization of http backend...")
		app.httpService = http_service.NewService(env, *httpConfig)
		logger.Info("initialization of http backend has been completed")
	}

	logger.Info("initialization of grpc backend...")

	grpcOptions := makeGRPCServiceConfig(cfg.GRPC, env.Metrics())
	app.grpcService, err = grpc_service.NewService(env, grpcOptions...)
	if err != nil {
		return nil, err
	}

	logger.Info("initialization of grpc backend has been completed")

	logger.Info("service initialization completed")
	return app, nil
}

func (a *Application) Run(runCtx context.Context) error {
	a.DefaultLogger().Debug("running application")

	defer a.DefaultLogger().Debug("stopping application")
	defer a.authCtx.cancel()
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

	if a.httpService != nil {
		group.Go(func() error {
			a.DefaultLogger().Info("running http service")
			return a.httpService.Run(ctx)
		})
	}

	group.Go(func() error {
		a.DefaultLogger().Info("running grpc service")
		return a.grpcService.Run(ctx)
	})

	return group.Wait()
}

func (a *Application) DefaultLogger() log.Logger {
	return logging.Logger().Logger()
}
