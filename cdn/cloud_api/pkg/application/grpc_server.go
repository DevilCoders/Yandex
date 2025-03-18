package application

import (
	"fmt"
	"net"

	grpcmiddleware "github.com/grpc-ecosystem/go-grpc-middleware"
	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials"
	"google.golang.org/grpc/reflection"

	"a.yandex-team.ru/cdn/cloud_api/pkg/application/xmiddleware"
	xgrpc "a.yandex-team.ru/cdn/cloud_api/pkg/application/xmiddleware/grpc"
	saltpb "a.yandex-team.ru/cdn/cloud_api/proto/saltapi"
	"a.yandex-team.ru/cdn/cloud_api/proto/userapi"
	cdnpb "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/cdn/v1/console"
	"a.yandex-team.ru/library/go/core/log"
)

type grpcServer struct {
	logger  log.Logger
	address string
	server  *grpc.Server
}

func (a *App) newGRPCServer(
	consolePB *consolePB,
	saltPB *saltPB,
	userapiPB *userapiPB,
) (*grpcServer, error) {
	options := []grpc.ServerOption{
		grpc.UnaryInterceptor(grpcmiddleware.ChainUnaryServer(
			xgrpc.RequestID(xmiddleware.RequestIDHeaderKey),
			xgrpc.Name,
			xgrpc.AccessLog(a.Logger),
			xgrpc.NewMetricMiddleware(
				a.Logger,
				a.MetricsRegistry.WithPrefix("grpc"),
			).RequestStat(),
			xgrpc.Recover(a.Logger),
		)),
	}

	tlsConfig := a.Config.ServerConfig.TLSConfig
	if tlsConfig.UseTLS {
		creds, err := credentials.NewServerTLSFromFile(tlsConfig.TLSCertPath, tlsConfig.TLSKeyPath)
		if err != nil {
			return nil, err
		}

		options = append(options, grpc.Creds(creds))
	}

	server := grpc.NewServer(options...)

	reflection.Register(server)

	// console api
	cdnpb.RegisterCacheServiceServer(server, consolePB.cache)
	cdnpb.RegisterOperationServiceServer(server, consolePB.operation)
	cdnpb.RegisterOriginServiceServer(server, consolePB.origin)
	cdnpb.RegisterOriginsGroupServiceServer(server, consolePB.originsGroup)
	cdnpb.RegisterProviderServiceServer(server, consolePB.provider)
	cdnpb.RegisterRawLogsServiceServer(server, consolePB.rawLogs)
	cdnpb.RegisterResourceServiceServer(server, consolePB.resource)
	cdnpb.RegisterResourceMetricsServiceServer(server, consolePB.resourceMetrics)
	cdnpb.RegisterResourceRulesServiceServer(server, consolePB.resourceRules)

	// salt api
	saltpb.RegisterSaltServiceServer(server, saltPB.salt)

	// user api
	userapi.RegisterResourceServiceServer(server, userapiPB.resource)
	userapi.RegisterOriginsGroupServiceServer(server, userapiPB.originsGroup)
	userapi.RegisterOriginServiceServer(server, userapiPB.origin)
	userapi.RegisterResourceRuleServiceServer(server, userapiPB.resourceRule)

	return &grpcServer{
		logger:  a.Logger,
		address: fmt.Sprintf(":%d", a.Config.ServerConfig.GRPCPort),
		server:  server,
	}, nil
}

func (s *grpcServer) run() error {
	s.logger.Infof("run grpc server: %s", s.address)

	listen, err := net.Listen("tcp", s.address)
	if err != nil {
		return fmt.Errorf("listen tcp on address: %s", s.address)
	}

	err = s.server.Serve(listen)
	if err != nil {
		return fmt.Errorf("stop grpc server: %w", err)
	}

	return nil
}

func (s *grpcServer) stop() error {
	s.logger.Infof("stopping grpc server: %s", s.address)
	s.server.GracefulStop()
	return nil
}
