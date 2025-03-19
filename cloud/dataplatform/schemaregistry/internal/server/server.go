package server

import (
	"context"
	"crypto/tls"
	"fmt"
	"net"
	"net/http"
	"os"
	"os/signal"
	"syscall"

	"github.com/dgraph-io/ristretto"
	"github.com/go-openapi/spec"
	grpc_middleware "github.com/grpc-ecosystem/go-grpc-middleware"
	"github.com/grpc-ecosystem/grpc-gateway/runtime"
	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials"
	"google.golang.org/grpc/health/grpc_health_v1"
	"google.golang.org/grpc/metadata"
	"google.golang.org/grpc/reflection"

	"a.yandex-team.ru/cloud/dataplatform/api/schemaregistry"
	"a.yandex-team.ru/cloud/dataplatform/internal/interceptors"
	"a.yandex-team.ru/cloud/dataplatform/internal/logger"
	"a.yandex-team.ru/cloud/dataplatform/internal/swag"
	"a.yandex-team.ru/cloud/dataplatform/schemaregistry/internal/config"
	"a.yandex-team.ru/cloud/dataplatform/schemaregistry/internal/server/api"
	"a.yandex-team.ru/cloud/dataplatform/schemaregistry/internal/server/namespace"
	"a.yandex-team.ru/cloud/dataplatform/schemaregistry/internal/server/schema"
	"a.yandex-team.ru/cloud/dataplatform/schemaregistry/internal/server/schema/provider"
	"a.yandex-team.ru/cloud/dataplatform/schemaregistry/internal/server/search"
	"a.yandex-team.ru/cloud/dataplatform/schemaregistry/internal/store/postgres"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/metrics/solomon"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/transfer_manager/go/pkg/auth"
	"a.yandex-team.ru/transfer_manager/go/pkg/buildinfo"
	"a.yandex-team.ru/transfer_manager/go/pkg/cleanup"
	"a.yandex-team.ru/transfer_manager/go/pkg/dbaas"
	"a.yandex-team.ru/transfer_manager/go/pkg/serverutil"
	"a.yandex-team.ru/transfer_manager/go/pkg/stats"
	"a.yandex-team.ru/transfer_manager/go/pkg/token"
)

func checkErr(err error) {
	if err != nil {
		logger.Log.Fatalf("error: %v", err)
	}
}

func skipAuth(info *grpc.UnaryServerInfo) bool {
	return info.FullMethod == "/grpc.health.v1.Health/Check"
}

func GetTransportCredentials(cfg *config.Config) credentials.TransportCredentials {
	if len(cfg.TLSServerPrivateCertificate) == 0 {
		return nil
	}
	pemBytes := cfg.TLSServerPrivateCertificate
	cert, err := tls.X509KeyPair([]byte(pemBytes), []byte(pemBytes))
	if err != nil {
		logger.Log.Fatalf("failed to parse certificate: %v", err)
		return nil
	}
	return credentials.NewServerTLSFromCert(&cert)
}

// Start Entry point to start the server
func Start(cfg *config.Config) {
	ctx := context.Background()

	store := postgres.NewStore(cfg.DB)
	namespaceService := namespace.Service{
		Repo: store,
	}
	cache, err := ristretto.NewCache(&ristretto.Config{
		NumCounters: 1000,
		MaxCost:     cfg.CacheSizeInMB << 20,
		BufferItems: 64,
	})
	if err != nil {
		panic(err)
	}
	schemaService := schema.NewService(store, provider.NewSchemaProvider(), namespaceService, cache)
	searchService := &search.Service{
		Repo: store,
	}

	tokenProvider, err := dbaas.NewTokenProvider(cfg.IAMURLBase)
	if err != nil {
		logger.Log.Fatalf("unable to initialize token provider: %v", err)
	}
	defer cleanup.Close(tokenProvider, logger.Log)

	loggerFactory := interceptors.WithContextLogger
	tokenInterceptor := token.NewInterceptor(token.NewInterceptorConfig(skipAuth), tokenProvider)
	authProvider, err := auth.NewProvider(ctx, cfg.AuthProvider, token.FromContext, loggerFactory)
	if err != nil {
		logger.Log.Fatalf("unable to initialize auth provider: %v", err)
		return
	}
	authInterceptor := auth.NewInterceptor(auth.NewInterceptorConfig(skipAuth, true), authProvider)
	instance := api.NewAPI(namespaceService, schemaService, searchService, authProvider)

	port := fmt.Sprintf(":%s", cfg.GRPCPort)
	mux := runtime.NewServeMux(
		runtime.WithLastMatchWins(),
		runtime.WithMetadata(func(context.Context, *http.Request) metadata.MD {
			return metadata.Pairs(interceptors.APIProtocolHeaderName, interceptors.APIProtocolHTTP)
		}),
	)

	// init grpc server
	opts := []grpc.ServerOption{
		grpc.UnaryInterceptor(
			grpc_middleware.ChainUnaryServer(
				interceptors.WithRequestInfo,
				interceptors.WithReqIDHeader,
				tokenInterceptor.Intercept,
				authInterceptor.Intercept,
				interceptors.WithCallLogger,
				auth.CheckAuth,
				interceptors.WithPanicaHandler,
				interceptors.WithUnwrapStatus,
			),
		),
		grpc.MaxRecvMsgSize(cfg.GRPC.MaxRecvMsgSizeInMB << 20),
		grpc.MaxSendMsgSize(cfg.GRPC.MaxSendMsgSizeInMB << 20),
	}
	if creds := GetTransportCredentials(cfg); creds != nil {
		opts = append(opts, grpc.Creds(creds))
	}
	// Create a gRPC server object
	srvr := grpc.NewServer(opts...)
	reflection.Register(srvr)
	schemaregistry.RegisterNamespaceServiceServer(srvr, api.NewNamespaceAPI(instance))
	schemaregistry.RegisterSchemaServiceServer(srvr, api.NewSchemaAPI(instance))
	schemaregistry.RegisterVersionServiceServer(srvr, api.NewVersionAPI(instance))
	schemaregistry.RegisterSearchServiceServer(srvr, instance)
	grpc_health_v1.RegisterHealthServer(srvr, instance)
	conn, err := grpc.DialContext(
		context.Background(),
		port,
		grpc.WithInsecure(),
	)
	if err != nil {
		logger.Log.Fatalf("Failed to dial server:%v", err)
	}
	//instance.RegisterSchemaHandlers(mux)

	checkErr(schemaregistry.RegisterNamespaceServiceHandler(ctx, mux, conn))
	checkErr(schemaregistry.RegisterSchemaServiceHandler(ctx, mux, conn))
	checkErr(schemaregistry.RegisterVersionServiceHandler(ctx, mux, conn))
	checkErr(schemaregistry.RegisterSearchServiceHandler(ctx, mux, conn))

	rootMux := http.NewServeMux()
	swag.NewSwagUI(
		swag.SpecResource(
			"swagger.json",
			swag.WithInfo(&spec.Info{
				VendorExtensible: *new(spec.VendorExtensible),
				InfoProps: spec.InfoProps{
					Description:    "Schema Registry",
					Title:          "Schema Registry",
					TermsOfService: "",
					Contact:        nil,
					License:        nil,
					Version:        buildinfo.ArcadiaVersion(),
				},
			}),
			swag.WithExternalDocs(&spec.ExternalDocumentation{
				Description: "",
				URL:         "https://wiki.yandex-team.ru/data-catalog/sr",
			}),
		),
		mux,
		swag.Title("Schema Registry"),
		swag.SpecResource("namespace_service.swagger.json"),
		swag.SpecResource("schema_service.swagger.json"),
		swag.SpecResource("search_service.swagger.json"),
		swag.SpecResource("version_service.swagger.json"),
	)
	rootMux.Handle("/", mux)
	rootMux.HandleFunc("/ping", serverutil.PingFunc)
	go func() {
		if err := http.ListenAndServe(fmt.Sprintf(":%s", cfg.HTTPPort), rootMux); err != nil {
			logger.Log.Errorf("failed to serve proxy: %v", err)
			srvr.Stop()
		}
	}()
	if err := serveGRPC(cfg, stats.NewServerMethods(solomon.NewRegistry(solomon.NewRegistryOpts())), srvr, logger.Log); err != nil {
		logger.Log.Fatal("server failed", log.Error(err))
	}
}

func serveGRPC(config *config.Config, serverMetrics *stats.ServerMethodStat, grpcServer *grpc.Server, logger log.Logger) error {
	serverMetrics.Init(grpcServer)
	listener, err := net.Listen("tcp", fmt.Sprintf(":%s", config.GRPCPort))
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
