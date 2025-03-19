package grpcserver

import (
	"context"
	"fmt"
	"net"
	"time"

	"google.golang.org/grpc"
	grpchealth "google.golang.org/grpc/health/grpc_health_v1"
	"google.golang.org/grpc/reflection"

	api "a.yandex-team.ru/cloud/mdb/cms/api/grpcapi/v1"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/cmsdb/pg"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/dom0discovery/conductorcache"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/dom0discovery/implementation"
	"a.yandex-team.ru/cloud/mdb/internal/app"
	as "a.yandex-team.ru/cloud/mdb/internal/compute/accessservice"
	grpcas "a.yandex-team.ru/cloud/mdb/internal/compute/accessservice/grpc"
	conductor "a.yandex-team.ru/cloud/mdb/internal/conductor/httpapi"
	dbmapi "a.yandex-team.ru/cloud/mdb/internal/dbm/restapi"
	"a.yandex-team.ru/cloud/mdb/internal/fs/fsnotify"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil/interceptors"
	"a.yandex-team.ru/cloud/mdb/internal/juggler/push/http"
	"a.yandex-team.ru/cloud/mdb/internal/ready"
	"a.yandex-team.ru/cloud/mdb/internal/retry"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type App struct {
	*app.App
	grpcServer   *grpc.Server
	grpcListener net.Listener
	*Config
}

func New(ctx context.Context) (*App, error) {
	cfg := DefaultConfig()
	baseApp, err := app.New(app.DefaultServiceOptions(cfg, fmt.Sprintf("%s.yaml", internal.AppCfgName))...)
	if err != nil {
		return nil, xerrors.Errorf("cannot make base app, %w", err)
	}

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

	cmsdb, err := pg.NewCMSDBWithConfigLoad(logger)
	if err != nil {
		return nil, xerrors.Errorf("cannot create cmsdb instance, %w", err)
	}

	if err = ready.Wait(ctx, cmsdb, &ready.DefaultErrorTester{Name: "cmsdb", L: logger}, time.Second); err != nil {
		return nil, xerrors.Errorf("failed to wait cmsdb, %w", err)
	}

	asClient, err := grpcas.NewClient(ctx, cfg.Auth.Addr, "MDB CMS", cfg.Auth.ClientConfig, logger)

	if err != nil {
		return nil, xerrors.Errorf("unable to init auth: %w", err)
	}
	auth := NewAuth(asClient, as.ResourceFolder(cfg.Auth.FolderID), cfg.Auth.Permission, cfg.Auth.SkipAuthErrors)

	dbm, err := dbmapi.New(cfg.Dbm, logger)
	if err != nil {
		return nil, xerrors.Errorf("new dbmapi client: %w", err)
	}
	cncl, err := conductor.New(cfg.Conductor, logger)
	if err != nil {
		return nil, xerrors.Errorf("new conductor client: %w", err)
	}
	d0dConfig := cfg.Cms.Dom0Discovery
	conductorCache := conductorcache.NewCache()
	dom0d := implementation.NewDom0DiscoveryImpl(d0dConfig.GroupsWhiteL, d0dConfig.GroupsBlackL, dbm, logger, conductorCache)

	mdb, err := pg.NewMetaDBWithConfigLoad(logger)
	if err != nil {
		return nil, xerrors.Errorf("cannot create metadb instance, %w", err)
	}

	pusher, err := http.NewLocalPusher(logger)
	if err != nil {
		return nil, xerrors.Errorf("cannot create juggler pusher: %w", err)
	}

	instanceService := NewInstanceService(
		cmsdb,
		dom0d,
		mdb,
		pusher,
		logger,
		auth,
		cncl,
		conductorCache,
		cfg,
	)
	api.RegisterInstanceOperationServiceServer(server, instanceService)
	api.RegisterInstanceServiceServer(server, instanceService)
	api.RegisterDutyServiceServer(server, instanceService)

	if err = instanceService.DoRefreshConductorCache(ctx); err != nil {
		return nil, xerrors.Errorf("init conductor cache: %w", err)
	}
	go instanceService.RefreshConductorCache(ctx)

	closer, err := fsnotify.NewFileWatcher(ctx, cfg.SLBCloseFile, logger)
	if err != nil {
		return nil, xerrors.Errorf("failed to create file watcher, %w", err)
	}
	health := HealthService{Cmsdb: cmsdb, Closer: closer, Logger: logger}
	grpchealth.RegisterHealthServer(server, &health)

	logger.Debug("Initializing GRPC listener", log.String("addr", cfg.GRPC.Addr))
	listener, err := grpcutil.Serve(server, cfg.GRPC.Addr, logger)
	if err != nil {
		return nil, fmt.Errorf("unable to init listener: %w", err)
	}

	return &App{
		App:          baseApp,
		grpcServer:   server,
		grpcListener: listener,
		Config:       cfg,
	}, nil
}

func (a *App) WaitForStop() {
	a.App.WaitForStop()
	if err := grpcutil.Shutdown(a.grpcServer, a.Config.GRPC.ShutdownTimeout); err != nil {
		a.L().Error("failed to shutdown gRPC server", log.Error(err))
	}
}

func (a *App) Run() {
	a.WaitForStop()
}
