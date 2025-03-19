package agentapp

import (
	"context"
	"net"

	"google.golang.org/grpc"
	"google.golang.org/grpc/reflection"

	api "a.yandex-team.ru/cloud/mdb/deploy/agent/internal/api/v1"
	"a.yandex-team.ru/cloud/mdb/deploy/agent/internal/pkg/agent"
	"a.yandex-team.ru/cloud/mdb/deploy/agent/internal/pkg/agent/call"
	"a.yandex-team.ru/cloud/mdb/deploy/agent/internal/pkg/agent/srv"
	"a.yandex-team.ru/cloud/mdb/deploy/agent/internal/pkg/datasource/s3"
	"a.yandex-team.ru/cloud/mdb/deploy/agent/internal/pkg/grpcserver"
	"a.yandex-team.ru/cloud/mdb/internal/app"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil/interceptors"
	"a.yandex-team.ru/cloud/mdb/internal/retry"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type App struct {
	*app.App
	grpcServer   *grpc.Server
	grpcListener net.Listener
	ag           *agent.Agent
	cfg          Config
}

func New() (*App, error) {
	cfg := DefaultConfig()
	a, err := app.New(
		app.WithConfig(&cfg),
		app.WithConfigLoad("agent.yaml"),
		app.WithSentry(),
		app.WithLoggerConstructor(app.DefaultServiceLoggerConstructor()),
		app.WithInstrumentation(),
	)
	if err != nil {
		return nil, xerrors.Errorf("init base app: %w", err)
	}
	options := []grpc.ServerOption{
		grpc.UnaryInterceptor(
			interceptors.ChainUnaryServerInterceptors(
				retry.New(cfg.Retry),
				cfg.ExposeErrorDebug,
				nil,
				interceptors.DefaultLoggingConfig(),
				a.L(),
			),
		),
		grpc.StreamInterceptor(
			interceptors.ChainStreamServerInterceptors(
				cfg.ExposeErrorDebug,
				nil,
				a.L(),
			),
		),
	}

	server := grpc.NewServer(options...)
	reflection.Register(server)
	saltService := grpcserver.New(a.L())
	api.RegisterCommandServiceServer(server, saltService)

	listener, err := grpcutil.Serve(server, cfg.GRPC.Addr, a.L(), grpcutil.WithUnixSocketMode(0600))
	if err != nil {
		return nil, xerrors.Errorf("init listener: %w", err)
	}

	s3source, err := s3.NewFromConfig(cfg.S3, a.L())
	if err != nil {
		return nil, xerrors.Errorf("init s3 source: %w", err)
	}

	srvManager, err := srv.New(cfg.Srv, a.L())
	if err != nil {
		return nil, xerrors.Errorf("init srv manager: %w", err)
	}

	return &App{
		App:          a,
		grpcServer:   server,
		grpcListener: listener,
		ag: agent.New(
			cfg.Agent,
			a.L(),
			saltService,
			s3source,
			call.New(a.L(), cfg.Call),
			srvManager,
		),
		cfg: cfg,
	}, nil
}

func (a *App) WaitForStop() {
	a.App.WaitForStop()
	if err := grpcutil.Shutdown(a.grpcServer, a.cfg.GRPC.ShutdownTimeout); err != nil {
		a.L().Error("failed to shutdown gRPC server", log.Error(err))
	}
}

func (a *App) Run() {
	serverDown := make(chan struct{})
	go func() {
		a.WaitForStop()
		close(serverDown)
	}()
	a.ag.Run(context.Background(), a.ShutdownContext().Done())
	// wait till gRPC server stops
	<-serverDown
}
