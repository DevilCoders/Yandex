package app

import (
	"context"
	"fmt"

	"golang.org/x/sync/errgroup"

	"a.yandex-team.ru/cloud/marketplace/license_server/internal/app/config"
	ydb_internal "a.yandex-team.ru/cloud/marketplace/license_server/internal/db/ydb"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/services/env"
	worker_service "a.yandex-team.ru/cloud/marketplace/license_server/internal/services/worker"
	"a.yandex-team.ru/cloud/marketplace/pkg/auth"
	"a.yandex-team.ru/cloud/marketplace/pkg/clients/billing-private"
	"a.yandex-team.ru/cloud/marketplace/pkg/ctxtools"
	"a.yandex-team.ru/cloud/marketplace/pkg/logging"
	"a.yandex-team.ru/cloud/marketplace/pkg/ydb"
	"a.yandex-team.ru/library/go/core/log"
)

const (
	workerServiceName = "mkt.license_server.worker"
)

type Worker struct {
	workerService *worker_service.Service

	authCtx struct {
		ctx    context.Context
		cancel context.CancelFunc
	}
}

func NewWorker(initCtx context.Context, configProxyPaths []string) (*Worker, error) {
	cfg, err := config.LoadWorkerConfig(initCtx, configProxyPaths)
	if err != nil {
		fmt.Println("failed load config", err)
		return nil, err
	}

	worker := &Worker{}

	if err := SetupLogging(cfg.Logger); err != nil {
		return nil, err
	}

	logger := logging.Named("worker")

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

	logger.Info("initialization of authentication")

	worker.authCtx.ctx, worker.authCtx.cancel = context.WithCancel(context.Background())
	defaultTokenAuth := auth.NewYCDefaultTokenAuthenticator(worker.authCtx.ctx)

	logger.Info("initialization of authentication")

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
		env.BackendsWithYCDefaultCredentials(defaultTokenAuth),
		env.BackendsWithYdb(
			ydb_internal.NewLicenseServerProvider(ydbConnection),
		),
	)
	envBuilder.WithCloudIDGeneratorPrefix(cfg.Prefix)

	env := envBuilder.
		WithHandlersLogger(logging.Named("handlers")).
		Build()

	logger.Info("constructing of context environment has been completed")

	err = makeAutorecreateWorkerServiceConfig(cfg.Worker)
	if err != nil {
		return nil, err
	}
	logger.Info("initialization of worker service...")
	worker.workerService, err = worker_service.NewService(env)
	if err != nil {
		return nil, err
	}
	logger.Info("initialization of worker service has been completed")

	logger.Info("service initialization completed")
	return worker, nil
}

func (a *Worker) Run(runCtx context.Context) error {
	a.DefaultLogger().Debug("running application")
	defer a.DefaultLogger().Debug("stopping application")
	defer a.authCtx.cancel()

	group, ctx := errgroup.WithContext(runCtx)
	ctx = ctxtools.WithStructuredLogger(ctx, a.DefaultLogger().Structured())

	group.Go(func() error {
		a.DefaultLogger().Info("running worker service")
		return a.workerService.Run(ctx)
	})

	return group.Wait()
}

func (a *Worker) DefaultLogger() log.Logger {
	return logging.Logger().Logger()
}
