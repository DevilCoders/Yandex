package internal

import (
	"net"
	"os"

	grpcprometheus "github.com/grpc-ecosystem/go-grpc-prometheus"
	"github.com/jonboulle/clockwork"
	"google.golang.org/grpc"
	grpchealth "google.golang.org/grpc/health/grpc_health_v1"
	"google.golang.org/grpc/reflection"

	consolev1 "a.yandex-team.ru/cloud/mdb/datacloud/private_api/datacloud/console/v1"
	apiv1 "a.yandex-team.ru/cloud/mdb/datacloud/private_api/datacloud/logs/v1"
	"a.yandex-team.ru/cloud/mdb/internal/app"
	"a.yandex-team.ru/cloud/mdb/internal/auth/grpcauth/iamauth"
	as "a.yandex-team.ru/cloud/mdb/internal/compute/accessservice"
	grpcas "a.yandex-team.ru/cloud/mdb/internal/compute/accessservice/grpc"
	"a.yandex-team.ru/cloud/mdb/internal/datatransfer"
	grpcdatatransfer "a.yandex-team.ru/cloud/mdb/internal/datatransfer/grpc"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil/interceptors"
	"a.yandex-team.ru/cloud/mdb/internal/metadb"
	pgmdb "a.yandex-team.ru/cloud/mdb/internal/metadb/pg"
	"a.yandex-team.ru/cloud/mdb/internal/retry"
	"a.yandex-team.ru/cloud/mdb/logs-api/internal/api"
	authprov "a.yandex-team.ru/cloud/mdb/logs-api/internal/auth/provider"
	"a.yandex-team.ru/cloud/mdb/logs-api/internal/health"
	"a.yandex-team.ru/cloud/mdb/logs-api/internal/logic"
	"a.yandex-team.ru/cloud/mdb/logs-api/internal/logsdb"
	"a.yandex-team.ru/cloud/mdb/logs-api/internal/logsdb/clickhouse"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/valid"
)

const (
	configName = "logs-api.yaml"
	userAgent  = "MDB Logs service"
)

type AppComponents struct {
	AccessService as.AccessService
	MetaDB        metadb.MetaDB
	LogsDB        logsdb.Backend
	Datatransfer  datatransfer.DataTransferService
}

type App struct {
	*app.App
	AppComponents

	Config       Config
	grpcServer   *grpc.Server
	grpcListener net.Listener
}

func Run(components AppComponents) {
	cfg := DefaultConfig()
	baseApp, err := app.New(app.DefaultServiceOptions(&cfg, configName)...)
	if err != nil {
		panic(err)
	}

	if !cfg.MetaDB.Password.FromEnv("METADB_PASSWORD") {
		baseApp.L().Info("METADB_PASSWORD is empty")
	}

	if !cfg.LogsDB.DB.Password.FromEnv("LOGSDB_PASSWORD") {
		baseApp.L().Info("LOGSDB_PASSWORD is empty")
	}

	a, err := NewApp(baseApp, cfg, components)
	if err != nil {
		baseApp.L().Errorf("failed to create app: %+v", err)
		os.Exit(1)
	}

	a.WaitForStop()
}

func NewApp(baseApp *app.App, cfg Config, components AppComponents) (*App, error) {
	vctx := valid.NewValidationCtx()
	if verr := valid.Struct(vctx, cfg); verr != nil {
		return nil, xerrors.Errorf("failed to validate config: %w", verr)
	}

	// Initialize access service client
	if components.AccessService == nil {
		asClient, err := grpcas.NewClient(baseApp.ShutdownContext(), cfg.AccessService.Addr, userAgent, cfg.AccessService.ClientConfig, baseApp.L())
		if err != nil {
			return nil, xerrors.Errorf("failed to initialize access service client: %w", err)
		}

		components.AccessService = asClient
	}

	// Initialize metadb client
	if components.MetaDB == nil {
		mdb, err := pgmdb.New(cfg.MetaDB, baseApp.L())
		if err != nil {
			return nil, xerrors.Errorf("failed to initialize metadb backend: %w", err)
		}

		components.MetaDB = mdb
	}

	// Init logsdb
	if components.LogsDB == nil {
		logsDB, err := clickhouse.New(cfg.LogsDB, baseApp.L())
		if err != nil {
			return nil, err
		}

		components.LogsDB = logsDB
	}

	if components.Datatransfer == nil {
		dt, err := grpcdatatransfer.New(
			baseApp.ShutdownContext(),
			cfg.DataTransfer.Addr,
			cfg.DataTransfer.ClientConfig,
			userAgent,
			baseApp.L(),
		)
		if err != nil {
			return nil, err
		}
		components.Datatransfer = dt
	}

	a := &App{
		App:           baseApp,
		AppComponents: components,
		Config:        cfg,
	}

	backoff := retry.New(cfg.API.Retry)
	grpcprometheus.EnableHandlingTimeHistogram()
	options := []grpc.ServerOption{
		grpc.UnaryInterceptor(
			interceptors.ChainUnaryServerInterceptors(
				backoff,
				cfg.API.ExposeErrorDebug,
				interceptors.NewNopReadOnlyChecker(),
				cfg.GRPC.Logging,
				a.L(),
				interceptors.NewUnaryServerInterceptorAuth(iamauth.NewIAMAuthTokenModel(), api.NeedAuthChecker),
			),
		),
		grpc.StreamInterceptor(
			interceptors.ChainStreamServerInterceptors(
				cfg.API.ExposeErrorDebug,
				interceptors.NewNopReadOnlyChecker(),
				a.L(),
				interceptors.NewStreamServerInterceptorAuth(iamauth.NewIAMAuthTokenModel(), api.NeedAuthChecker),
			),
		),
	}

	// Init auth
	authenticator := authprov.NewAuthProvider(components.MetaDB, components.AccessService, components.Datatransfer, a.L())

	// Init logic
	logs := logic.NewLogs(cfg.Logic, authenticator, components.LogsDB, clockwork.NewRealClock())

	// Create server
	server := grpc.NewServer(options...)

	// Init Health services
	hs := api.NewHealthService(health.NewHealth(components.MetaDB, components.Datatransfer, components.LogsDB), baseApp.L())
	grpchealth.RegisterHealthServer(server, hs)

	// Init log service
	logsService := api.NewLogsService(logs, a.App.L())
	apiv1.RegisterLogServiceServer(server, logsService)

	// Init console log service
	consoleLogsService := api.NewConsoleLogsService(logs)
	consolev1.RegisterLogServiceServer(server, consoleLogsService)

	// Register server for reflection
	reflection.Register(server)
	// Register server for metrics
	grpcprometheus.Register(server)

	a.grpcServer = server

	listener, err := grpcutil.Serve(a.grpcServer, a.Config.GRPC.Addr, a.App.L())
	if err != nil {
		return nil, err
	}

	a.grpcServer = server
	a.grpcListener = listener
	return a, nil
}

func (a *App) WaitForStop() {
	a.App.WaitForStop()
	if err := grpcutil.Shutdown(a.grpcServer, a.Config.GRPC.ShutdownTimeout); err != nil {
		a.L().Error("failed to shutdown gRPC server", log.Error(err))
	}
}

func (a *App) GRPCPort() int {
	return a.grpcListener.Addr().(*net.TCPAddr).Port
}
