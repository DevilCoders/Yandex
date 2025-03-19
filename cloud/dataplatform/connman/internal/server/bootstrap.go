package server

import (
	"context"
	"fmt"
	"net"
	"net/http"
	"os"
	"os/signal"
	"syscall"

	grpc_middleware "github.com/grpc-ecosystem/go-grpc-middleware"
	"github.com/grpc-ecosystem/grpc-gateway/runtime"
	"google.golang.org/grpc"
	"google.golang.org/grpc/health"
	"google.golang.org/grpc/health/grpc_health_v1"
	"google.golang.org/grpc/metadata"

	"a.yandex-team.ru/cloud/dataplatform/api/connman"
	"a.yandex-team.ru/cloud/dataplatform/connman/internal/configuration"
	"a.yandex-team.ru/cloud/dataplatform/connman/internal/encryption"
	"a.yandex-team.ru/cloud/dataplatform/connman/internal/serialization"
	"a.yandex-team.ru/cloud/dataplatform/connman/internal/storage"
	"a.yandex-team.ru/cloud/dataplatform/connman/internal/view"
	"a.yandex-team.ru/cloud/dataplatform/internal/interceptors"
	internal_metrics "a.yandex-team.ru/cloud/dataplatform/internal/metrics"
	"a.yandex-team.ru/cloud/dataplatform/internal/swag"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/metrics"
	"a.yandex-team.ru/library/go/core/metrics/solomon"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/transfer_manager/go/pkg/auth"
	"a.yandex-team.ru/transfer_manager/go/pkg/cleanup"
	"a.yandex-team.ru/transfer_manager/go/pkg/dbaas"
	"a.yandex-team.ru/transfer_manager/go/pkg/grpcutil"
	"a.yandex-team.ru/transfer_manager/go/pkg/serverutil"
	"a.yandex-team.ru/transfer_manager/go/pkg/stats"
	"a.yandex-team.ru/transfer_manager/go/pkg/token"
)

func Start(ctx context.Context, config *configuration.Config, logger log.Logger) error {
	loggerFactory := interceptors.WithContextLogger

	pg, err := dbaas.NewPgHAFromMDB(config.Pg.Database, string(config.Pg.User), string(config.Pg.Password), config.Pg.MDBClusterID, "")
	if err != nil {
		return xerrors.Errorf("unable to initialize pg: %w", err)
	}
	defer cleanup.Close(pg, logger)

	encryptionProvider, err := encryption.NewKmsProvider(ctx, config)
	if err != nil {
		return xerrors.Errorf("unable to initialize kms: %w", err)
	}

	serializer := serialization.NewSerializer(encryptionProvider)

	store := storage.NewPgStorage(pg, serializer, loggerFactory)

	lockboxDecryptor, err := encryption.NewLockboxDecryptor(ctx)
	if err != nil {
		return xerrors.Errorf("unable to initialize lockbox: %w", err)
	}

	viewer := view.NewViewer(lockboxDecryptor)

	tokenProvider, err := token.NewProvider(ctx, &config.TokenProvider)
	if err != nil {
		return xerrors.Errorf("unable to initialize token provider: %w", err)
	}
	defer cleanup.Close(tokenProvider, logger)

	tokenInterceptor := token.NewInterceptor(token.NewInterceptorConfig(skipAuth), tokenProvider)

	authProvider, err := auth.NewProvider(ctx, &config.AuthProvider, token.FromContext, loggerFactory)
	if err != nil {
		return xerrors.Errorf("unable to initialize auth provider: %w", err)
	}
	defer cleanup.Close(authProvider, logger)

	authInterceptor := auth.NewInterceptor(auth.NewInterceptorConfig(skipAuth, true), authProvider)

	metricsRegistry := newMetricsRegistry()

	connectionServer := NewConnectionServer(store, authProvider, viewer)

	serverMetrics := stats.NewServerMethods(metricsRegistry)

	grpcServer := newServer(serverMetrics, tokenInterceptor, authInterceptor)

	internal_metrics.NewPs(metricsRegistry)

	connman.RegisterConnectionServiceServer(grpcServer, connectionServer)

	registerHealthServer(grpcServer)

	err = serveHTTP(ctx, config, grpcServer, logger)
	if err != nil {
		return xerrors.Errorf("unable to serve http: %w", err)
	}

	err = serveGRPC(config, serverMetrics, grpcServer, logger)
	if err != nil {
		return xerrors.Errorf("unable to serve grpc: %w", err)
	}

	return nil
}

func skipAuth(info *grpc.UnaryServerInfo) bool {
	return info.FullMethod == "/grpc.health.v1.Health/Check"
}

func newMetricsRegistry() metrics.Registry {
	opts := solomon.NewRegistryOpts().
		SetSeparator('.').
		SetUseNameTag(true)
	return solomon.NewRegistry(opts)
}

func newServer(serverMetrics *stats.ServerMethodStat, tokenInterceptor grpcutil.UnaryServerInterceptor, authInterceptor grpcutil.UnaryServerInterceptor) *grpc.Server {
	return grpc.NewServer(
		grpc.UnaryInterceptor(grpc_middleware.ChainUnaryServer(
			interceptors.WithRequestMetrics(serverMetrics),
			interceptors.WithDefaultStatus,
			interceptors.WithRequestInfo,
			interceptors.WithReqIDHeader,
			tokenInterceptor.Intercept,
			authInterceptor.Intercept,
			interceptors.WithCallLogger,
			auth.CheckAuth,
			interceptors.WithPanicaHandler,
			interceptors.WithUnwrapStatus,
		)),
	)
}

func registerHealthServer(grpcServer *grpc.Server) {
	healthServer := health.NewServer()
	healthServer.SetServingStatus("yandex.cloud.priv.connman.v1.ConnectionService", grpc_health_v1.HealthCheckResponse_SERVING)
	grpc_health_v1.RegisterHealthServer(grpcServer, healthServer)
}

func serveHTTP(ctx context.Context, config *configuration.Config, grpcServer *grpc.Server, logger log.Logger) error {
	mux := runtime.NewServeMux(
		runtime.WithLastMatchWins(),
		runtime.WithMetadata(func(context.Context, *http.Request) metadata.MD {
			return metadata.Pairs(interceptors.APIProtocolHeaderName, interceptors.APIProtocolHTTP)
		}),
	)
	err := connman.RegisterConnectionServiceHandlerFromEndpoint(
		ctx, mux, fmt.Sprintf(":%d", config.GrpcPort), []grpc.DialOption{grpc.WithInsecure()})
	if err != nil {
		return xerrors.Errorf("unable to register connection service handler from endpoint: %w", err)
	}

	swag.NewSwagUI(
		swag.SpecResource("swagger.json"),
		mux,
		swag.SpecResource("connection_service.swagger.json"),
	)

	rootMux := http.NewServeMux()
	rootMux.Handle("/", mux)
	rootMux.HandleFunc("/ping", serverutil.PingFunc)
	go func() {
		if err := http.ListenAndServe(fmt.Sprintf(":%d", config.HTTPPort), rootMux); err != nil {
			logger.Error("failed to serve proxy", log.Error(err))
			grpcServer.Stop()
		}
	}()

	return nil
}

func serveGRPC(config *configuration.Config, serverMetrics *stats.ServerMethodStat, grpcServer *grpc.Server, logger log.Logger) error {
	serverMetrics.Init(grpcServer)

	listener, err := net.Listen("tcp", fmt.Sprintf(":%d", config.GrpcPort))
	if err != nil {
		return xerrors.Errorf("unable to start grpc listener: %w", err)
	}
	defer cleanup.Close(listener, logger)

	stopSignals := make(chan os.Signal)
	signal.Notify(stopSignals, syscall.SIGINT, syscall.SIGTERM)
	go func() {
		<-stopSignals
		grpcServer.Stop()
	}()

	err = grpcServer.Serve(listener)
	if err != nil {
		return xerrors.Errorf("failed to serve grpc: %w", err)
	}

	return nil
}
