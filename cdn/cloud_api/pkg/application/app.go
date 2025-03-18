package application

import (
	"context"
	"fmt"
	"syscall"

	grpcretry "github.com/grpc-ecosystem/go-grpc-middleware/retry"
	lru "github.com/hashicorp/golang-lru"
	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials"
	"google.golang.org/grpc/keepalive"
	"google.golang.org/grpc/metadata"
	gormpostgres "gorm.io/driver/postgres"
	"gorm.io/gorm"
	"gorm.io/plugin/dbresolver"

	"a.yandex-team.ru/cdn/cloud_api/pkg/application/closer"
	"a.yandex-team.ru/cdn/cloud_api/pkg/application/xmiddleware"
	"a.yandex-team.ru/cdn/cloud_api/pkg/configuration"
	"a.yandex-team.ru/cdn/cloud_api/pkg/handler/adminhandler"
	"a.yandex-team.ru/cdn/cloud_api/pkg/handler/consoleapi"
	"a.yandex-team.ru/cdn/cloud_api/pkg/handler/grpcutil"
	"a.yandex-team.ru/cdn/cloud_api/pkg/handler/saltapi"
	"a.yandex-team.ru/cdn/cloud_api/pkg/handler/userapi"
	"a.yandex-team.ru/cdn/cloud_api/pkg/service/adminservice"
	"a.yandex-team.ru/cdn/cloud_api/pkg/service/auth"
	"a.yandex-team.ru/cdn/cloud_api/pkg/service/originservice"
	"a.yandex-team.ru/cdn/cloud_api/pkg/service/resourceservice"
	storagepkg "a.yandex-team.ru/cdn/cloud_api/pkg/storage"
	"a.yandex-team.ru/cdn/cloud_api/pkg/storage/postgres"
	saltpb "a.yandex-team.ru/cdn/cloud_api/proto/saltapi"
	userapipb "a.yandex-team.ru/cdn/cloud_api/proto/userapi"
	cdnpb "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/cdn/v1/console"
	cloudauth "a.yandex-team.ru/cloud/iam/accessservice/client/iam-access-service-client-go/v1"
	"a.yandex-team.ru/library/go/certifi"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/metrics/solomon"
)

type App struct {
	Config          *configuration.Config
	Logger          log.Logger
	MetricsRegistry *solomon.Registry
	Closer          *closer.Closer
}

func New(cfg *configuration.Config, logger log.Logger, registry *solomon.Registry) *App {
	return &App{
		Config:          cfg,
		Logger:          logger,
		MetricsRegistry: registry,
		Closer:          closer.NewCloser(logger, cfg.ServerConfig.ShutdownTimeout, syscall.SIGINT, syscall.SIGTERM),
	}
}

type consolePB struct {
	cache           cdnpb.CacheServiceServer
	operation       cdnpb.OperationServiceServer
	origin          cdnpb.OriginServiceServer
	originsGroup    cdnpb.OriginsGroupServiceServer
	provider        cdnpb.ProviderServiceServer
	rawLogs         cdnpb.RawLogsServiceServer
	resource        cdnpb.ResourceServiceServer
	resourceMetrics cdnpb.ResourceMetricsServiceServer
	resourceRules   cdnpb.ResourceRulesServiceServer
}

type saltPB struct {
	salt saltpb.SaltServiceServer
}

type userapiPB struct {
	resource     userapipb.ResourceServiceServer
	resourceRule userapipb.ResourceRuleServiceServer
	origin       userapipb.OriginServiceServer
	originsGroup userapipb.OriginsGroupServiceServer
}

func (a *App) Run() error {
	ctx, cancelFunc := context.WithCancel(context.Background())
	a.Closer.Add(func() error {
		cancelFunc()
		return nil
	})

	storage, err := constructStorage(a.Config.StorageConfig, a.Logger)
	if err != nil {
		return fmt.Errorf("construct storage: %w", err)
	}

	authClient, err := constructAuthClient(ctx, a.Config.AuthClientConfig, a.Closer)
	if err != nil {
		return fmt.Errorf("construct auth service: %w", err)
	}

	resourceFolderIDCache, err := lru.New(a.Config.APIConfig.FolderIDCacheSize)
	if err != nil {
		return fmt.Errorf("create resource folderID cache: %w", err)
	}

	originsGroupFolderIDCache, err := lru.New(a.Config.APIConfig.FolderIDCacheSize)
	if err != nil {
		return fmt.Errorf("create origin groups folderID cache: %w", err)
	}

	consolePB := constructConsolePB(a.Config.APIConfig.ConsoleAPIConfig, a.Logger, authClient, storage, resourceFolderIDCache, originsGroupFolderIDCache)
	saltPB := constructSaltPB(a.Logger, storage)
	userapiPB, err := constructUserapiPB(a.Config.APIConfig.UserAPIConfig, a.Logger, authClient, storage, resourceFolderIDCache, originsGroupFolderIDCache)
	if err != nil {
		return fmt.Errorf("construct userApi: %w", err)
	}

	adminHandler := constructAdminHandler(a.Config.DatabaseGCConfig, a.Logger, storage)

	grpcServer, err := a.newGRPCServer(consolePB, saltPB, userapiPB)
	if err != nil {
		return fmt.Errorf("construct grpc server: %w", err)
	}
	a.Closer.Add(grpcServer.stop)

	httpServer, err := a.newHTTPServer(ctx, saltPB, userapiPB, adminHandler)
	if err != nil {
		return fmt.Errorf("new http server: %w", err)
	}
	a.Closer.Add(httpServer.stop)

	a.Closer.Run(httpServer.run)
	a.Closer.Run(grpcServer.run)

	a.Closer.Wait()

	return nil
}

func constructConsolePB(
	cfg configuration.ConsoleAPIConfig,
	logger log.Logger,
	authClient grpcutil.AuthClient,
	storage storagepkg.Storage,
	resourceFolderIDCache *lru.Cache,
	originsGroupFolderIDCache *lru.Cache,
) *consolePB {
	originService := &originservice.Service{
		Logger:               logger,
		Storage:              storage,
		AutoActivateEntities: cfg.EnableAutoActivateEntities,
	}

	resourceService := &resourceservice.Service{
		Logger:               logger,
		ResourceGenerator:    resourceservice.NewResourceGenerator(),
		Storage:              storage,
		AutoActivateEntities: cfg.EnableAutoActivateEntities,
	}

	return &consolePB{
		cache: &consoleapi.CacheServiceHandler{
			Logger: logger,
		},
		operation: &consoleapi.OperationService{
			Logger: logger,
		},
		origin: &consoleapi.OriginServiceHandler{
			Logger:        logger,
			AuthClient:    authClient,
			OriginService: originService,
			GroupService:  originService,
			Cache:         originsGroupFolderIDCache,
		},
		originsGroup: &consoleapi.OriginsGroupServiceHandler{
			Logger:             logger,
			AuthClient:         authClient,
			OriginGroupService: originService,
			Cache:              originsGroupFolderIDCache,
		},
		provider: &consoleapi.ProviderServiceHandler{
			Logger: logger,
		},
		rawLogs: &consoleapi.RawLogsServiceHandler{},
		resource: &consoleapi.ResourceServiceHandler{
			Logger:          logger,
			AuthClient:      authClient,
			ResourceService: resourceService,
			Cache:           resourceFolderIDCache,
		},
		resourceMetrics: &consoleapi.ResourceMetricsServiceHandler{
			Logger:          logger,
			AuthClient:      authClient,
			ResourceService: resourceService,
		},
		resourceRules: &consoleapi.ResourceRulesServiceHandler{
			Logger:          logger,
			AuthClient:      authClient,
			ResourceService: resourceService,
			RuleService:     resourceService,
			Cache:           resourceFolderIDCache,
		},
	}
}

func constructSaltPB(logger log.Logger, storage storagepkg.Storage) *saltPB {
	originService := &originservice.Service{
		Logger:               logger,
		Storage:              storage,
		AutoActivateEntities: true,
	}

	resourceService := &resourceservice.Service{
		Logger:               logger,
		ResourceGenerator:    resourceservice.NewResourceGenerator(),
		Storage:              storage,
		AutoActivateEntities: true,
	}

	return &saltPB{
		salt: &saltapi.SaltServiceHandler{
			Logger:             logger,
			OriginGroupService: originService,
			ResourceService:    resourceService,
			RuleService:        resourceService,
		},
	}
}

func constructUserapiPB(
	cfg configuration.UserAPIConfig,
	logger log.Logger,
	authClient grpcutil.AuthClient,
	storage storagepkg.Storage,
	resourceFolderIDCache *lru.Cache,
	originsGroupFolderIDCache *lru.Cache,
) (*userapiPB, error) {
	originService := &originservice.Service{
		Logger:               logger,
		Storage:              storage,
		AutoActivateEntities: cfg.EnableAutoActivateEntities,
	}

	resourceService := &resourceservice.Service{
		Logger:               logger,
		ResourceGenerator:    resourceservice.NewResourceGenerator(),
		Storage:              storage,
		AutoActivateEntities: cfg.EnableAutoActivateEntities,
	}

	return &userapiPB{
		resource: &userapi.ResourceServiceHandler{
			Logger:          logger,
			AuthClient:      authClient,
			ResourceService: resourceService,
			RuleService:     resourceService,
			Cache:           resourceFolderIDCache,
		},
		resourceRule: &userapi.ResourceRuleServiceHandler{
			Logger:          logger,
			AuthClient:      authClient,
			ResourceService: resourceService,
			RuleService:     resourceService,
			Cache:           resourceFolderIDCache,
		},
		origin: &userapi.OriginServiceHandler{
			Logger:        logger,
			AuthClient:    authClient,
			OriginService: originService,
			GroupService:  originService,
			Cache:         originsGroupFolderIDCache,
		},
		originsGroup: &userapi.OriginsGroupServiceHandler{
			Logger:        logger,
			AuthClient:    authClient,
			OriginService: originService,
			Cache:         originsGroupFolderIDCache,
		},
	}, nil
}

func constructAdminHandler(cfg configuration.DatabaseGCConfig, logger log.Logger, pgStorage *postgres.Storage) *adminhandler.Handler {
	return &adminhandler.Handler{
		Logger: logger,
		AdminService: &adminservice.ServiceImpl{
			Logger:           logger,
			AdminStorage:     pgStorage,
			Storage:          pgStorage,
			DatabaseGCConfig: cfg,
		},
	}
}

// TODO: close connection, configure connection pool?
func constructStorage(cfg configuration.StorageConfig, logger log.Logger) (*postgres.Storage, error) {
	newDialector := func(dsn string) gorm.Dialector {
		return gormpostgres.New(gormpostgres.Config{
			DSN:                  dsn,
			PreferSimpleProtocol: true,
		})
	}

	db, err := gorm.Open(newDialector(cfg.DSN), &gorm.Config{
		Logger: postgres.NewLogger(logger),
	})
	if err != nil {
		return nil, fmt.Errorf("master open connection: %w", err)
	}

	sqlDB, err := db.DB()
	if err != nil {
		return nil, fmt.Errorf("get sql pool for master: %w", err)
	}
	sqlDB.SetConnMaxIdleTime(cfg.MaxIdleTime)
	sqlDB.SetConnMaxLifetime(cfg.MaxLifeTime)
	sqlDB.SetMaxIdleConns(cfg.MaxIdleConn)
	sqlDB.SetMaxOpenConns(cfg.MaxOpenConn)

	if cfg.ReplicaDSN != "" {
		err = db.Use(
			dbresolver.Register(dbresolver.Config{Replicas: []gorm.Dialector{newDialector(cfg.ReplicaDSN)}}).
				SetConnMaxIdleTime(cfg.MaxIdleTime).
				SetConnMaxLifetime(cfg.MaxLifeTime).
				SetMaxIdleConns(cfg.MaxIdleConn).
				SetMaxOpenConns(cfg.MaxOpenConn),
		)
		if err != nil {
			return nil, fmt.Errorf("replicas open connection: %w", err)
		}
	}

	return postgres.NewStorage(db), nil
}

func constructAuthClient(ctx context.Context, cfg configuration.AuthClientConfig, closer *closer.Closer) (grpcutil.AuthClient, error) {
	if cfg.EnableMock {
		return &grpcutil.MockAuthClient{}, nil
	}

	ctx, cancelFunc := context.WithTimeout(ctx, cfg.ConnectionInitTimeout)
	defer cancelFunc()

	certPool, err := certifi.NewCertPool()
	if err != nil {
		return nil, fmt.Errorf("new cert pool: %w", err)
	}
	tls := credentials.NewClientTLSFromCert(certPool, "")

	conn, err := grpc.DialContext(ctx, cfg.Endpoint,
		grpc.WithBlock(),
		grpc.WithTransportCredentials(tls),
		grpc.WithUserAgent(cfg.UserAgent),
		grpc.WithKeepaliveParams(keepalive.ClientParameters{
			Time:                cfg.KeepAliveTime,
			Timeout:             cfg.KeepAliveTimeout,
			PermitWithoutStream: true,
		}),
		grpc.WithChainUnaryInterceptor(
			func(ctx context.Context, method string, req, reply interface{}, cc *grpc.ClientConn, invoker grpc.UnaryInvoker, opts ...grpc.CallOption) error {
				requestID := xmiddleware.GetRequestID(ctx)
				if requestID != "" {
					ctx = metadata.AppendToOutgoingContext(ctx, cloudauth.RequestID, requestID)
				}

				return invoker(ctx, method, req, reply, cc, opts...)
			},
			grpcretry.UnaryClientInterceptor(
				grpcretry.WithMax(cfg.MaxRetries),
				grpcretry.WithPerRetryTimeout(cfg.RetryTimeout),
			),
		),
	)
	if err != nil {
		return nil, fmt.Errorf("connection dial context: %w", err)
	}
	closer.Add(conn.Close)

	return &grpcutil.AuthClientImpl{
		Service: auth.NewCloudIAMAccessService(conn),
	}, nil
}
