package api

import (
	"fmt"
	"net"
	"time"

	"google.golang.org/grpc"
	grpchealth "google.golang.org/grpc/health/grpc_health_v1"
	"google.golang.org/grpc/reflection"

	consoleapi "a.yandex-team.ru/cloud/mdb/datacloud/private_api/datacloud/network/console/v1"
	api "a.yandex-team.ru/cloud/mdb/datacloud/private_api/datacloud/network/v1"
	"a.yandex-team.ru/cloud/mdb/internal/app"
	grpcas "a.yandex-team.ru/cloud/mdb/internal/compute/accessservice/grpc"
	"a.yandex-team.ru/cloud/mdb/internal/fs/fsnotify"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil/interceptors"
	"a.yandex-team.ru/cloud/mdb/internal/httputil"
	"a.yandex-team.ru/cloud/mdb/internal/ready"
	"a.yandex-team.ru/cloud/mdb/internal/retry"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/api/auth"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/api/config"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/api/console"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/api/health"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/api/network"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/api/networkconnection"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/api/validation"
	awsvalidator "a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/api/validation/aws"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/providers/aws"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb/models"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb/pg"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type App struct {
	*app.App
	grpcServer   *grpc.Server
	grpcListener net.Listener
	cfg          *config.Config
}

func New() (*App, error) {
	cfg := config.DefaultConfig()
	baseApp, err := app.New(app.DefaultServiceOptions(cfg, "mdb-vpc-api.yaml")...)
	if err != nil {
		return nil, xerrors.Errorf("cannot make base app, %w", err)
	}
	ctx := baseApp.ShutdownContext()

	logger := baseApp.L()
	backoff := retry.New(cfg.Retry)
	options := []grpc.ServerOption{
		grpc.UnaryInterceptor(
			interceptors.ChainUnaryServerInterceptors(
				backoff,
				true,
				nil,
				interceptors.DefaultLoggingConfig(),
				logger,
			),
		),
	}

	server := grpc.NewServer(options...)
	reflection.Register(server)

	db, err := pg.New(cfg.VPCDB, logger)
	if err != nil {
		return nil, xerrors.Errorf("can not create vpcdb instance: %w", err)
	}

	if err = ready.WaitWithTimeout(ctx, 10*time.Second, db, &ready.DefaultErrorTester{Name: "vpcdb", L: logger}, time.Second); err != nil {
		return nil, xerrors.Errorf("failed to wait vpcdb, %w", err)
	}

	asClient, err := grpcas.NewClient(ctx, cfg.Auth.Addr, "MDB VPC", cfg.Auth.Config, logger)
	if err != nil {
		return nil, xerrors.Errorf("unable to init auth: %w", err)
	}

	authCli := auth.NewAuth(asClient)

	creds := aws.NewSACreds(cfg.AWSSA)
	httpClient, err := httputil.NewClient(cfg.AWS.HTTP, logger)
	if err != nil {
		logger.Fatal("could not create http client", log.Error(err))
	}
	ses := aws.NewSession(creds, "", httpClient.Client)

	validators := map[models.Provider]validation.Validator{
		models.ProviderAWS: awsvalidator.NewValidator(ses, httpClient.Client, cfg.AWS.DataplaneRoleArn),
	}

	networkService := network.NewService(db, cfg.AWS, authCli, validators)
	api.RegisterNetworkServiceServer(server, networkService)

	networkConnectionService := networkconnection.NewService(db, authCli, validators)
	api.RegisterNetworkConnectionServiceServer(server, networkConnectionService)

	cloudService := console.NewService(cfg.Regions)
	consoleapi.RegisterCloudServiceServer(server, cloudService)

	closer, err := fsnotify.NewFileWatcher(ctx, cfg.SLBCloseFile, logger)
	if err != nil {
		return nil, xerrors.Errorf("failed to create file watcher, %w", err)
	}
	grpchealth.RegisterHealthServer(server, health.NewService(db, closer, logger))

	logger.Debug("Initializing GRPC listener", log.String("addr", cfg.GRPC.Addr))
	listener, err := grpcutil.Serve(server, cfg.GRPC.Addr, logger)
	if err != nil {
		return nil, fmt.Errorf("unable to init listener: %w", err)
	}

	return &App{
		App:          baseApp,
		grpcServer:   server,
		grpcListener: listener,
		cfg:          cfg,
	}, nil
}

func (a *App) Run() {
	a.WaitForStop()
	if err := grpcutil.Shutdown(a.grpcServer, a.cfg.GRPC.ShutdownTimeout); err != nil {
		a.L().Error("failed to shutdown gRPC server", log.Error(err))
	}
}
