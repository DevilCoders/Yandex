package internal

import (
	"net"
	"os"

	grpc_prometheus "github.com/grpc-ecosystem/go-grpc-prometheus"
	"google.golang.org/grpc"
	grpchealth "google.golang.org/grpc/health/grpc_health_v1"
	"google.golang.org/grpc/reflection"

	"a.yandex-team.ru/cloud/mdb/internal/app"
	"a.yandex-team.ru/cloud/mdb/internal/auth/grpcauth/iamauth"
	as "a.yandex-team.ru/cloud/mdb/internal/compute/accessservice"
	grpcas "a.yandex-team.ru/cloud/mdb/internal/compute/accessservice/grpc"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil/interceptors"
	"a.yandex-team.ru/cloud/mdb/internal/retry"
	"a.yandex-team.ru/cloud/mdb/mdb-pillar-secrets/internal/api"
	authprov "a.yandex-team.ru/cloud/mdb/mdb-pillar-secrets/internal/auth/provider"
	"a.yandex-team.ru/cloud/mdb/mdb-pillar-secrets/internal/crypto"
	"a.yandex-team.ru/cloud/mdb/mdb-pillar-secrets/internal/crypto/nacl"
	"a.yandex-team.ru/cloud/mdb/mdb-pillar-secrets/internal/health"
	"a.yandex-team.ru/cloud/mdb/mdb-pillar-secrets/internal/logic"
	"a.yandex-team.ru/cloud/mdb/mdb-pillar-secrets/internal/metadb"
	pgmdb "a.yandex-team.ru/cloud/mdb/mdb-pillar-secrets/internal/metadb/pg"
	apiv1 "a.yandex-team.ru/cloud/mdb/mdb-pillar-secrets/proto/mdb/pillarsecrets/v1"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/valid"
)

const (
	configName = "mdb-pillar-secrets.yaml"
	userAgent  = "MDB Pillar Secrets API"
)

type AppComponents struct {
	AccessService as.AccessService
	MetaDB        metadb.MetaDB
	Crypto        crypto.Crypto
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

	if !cfg.Crypto.PrivateKey.FromEnv("CRYPTO_PRIVATE_KEY") {
		baseApp.L().Info("CRYPTO_PRIVATE_KEY is empty")
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

	// Initialize crypto provider
	if components.Crypto == nil {
		cryptoClient, err := nacl.New(cfg.Crypto.PeersPublicKey, cfg.Crypto.PrivateKey.Unmask())
		if err != nil {
			return nil, xerrors.Errorf("failed to initialize crypto provider: %w", err)
		}

		components.Crypto = cryptoClient
	}

	a := &App{
		App:           baseApp,
		AppComponents: components,
		Config:        cfg,
	}

	backoff := retry.New(cfg.API.Retry)
	grpc_prometheus.EnableHandlingTimeHistogram()
	options := []grpc.ServerOption{
		grpc.UnaryInterceptor(
			interceptors.ChainUnaryServerInterceptors(
				backoff,
				cfg.API.ExposeErrorDebug,
				interceptors.NewNopReadOnlyChecker(),
				cfg.GRPC.Logging,
				a.L(),
				interceptors.NewUnaryServerInterceptorAuth(iamauth.NewIAMAuthTokenModel(), api.IsMDB),
			),
		),
	}

	// Init auth
	authenticator := authprov.NewAuthProvider(components.MetaDB, components.AccessService, a.L())

	// Init logic
	pillarSecrets := logic.NewPillarSecret(authenticator, a.Crypto)

	// Create server
	server := grpc.NewServer(options...)

	// Init Health services
	hs := api.NewHealthService(health.NewHealth(components.MetaDB))
	grpchealth.RegisterHealthServer(server, hs)

	// Init services
	pillarSecretsService := api.NewPillarSecretService(pillarSecrets)
	apiv1.RegisterPillarSecretServiceServer(server, pillarSecretsService)

	// Register server for reflection
	reflection.Register(server)
	// Register server for metrics
	grpc_prometheus.Register(server)

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
