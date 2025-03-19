package grpc

import (
	"context"
	"net"

	"golang.org/x/sync/errgroup"
	"google.golang.org/grpc"
	"google.golang.org/grpc/reflection"

	"a.yandex-team.ru/cloud/marketplace/lich/internal/services/env"
	"a.yandex-team.ru/cloud/marketplace/pkg/ctxtools"
	"a.yandex-team.ru/library/go/core/log"
)

type Service struct {
	grpcServer *grpc.Server

	listenEndpoint string
}

func NewService(env *env.Env, options ...Option) (*Service, error) {
	var (
		configurator configurator
	)

	for i := range options {
		options[i](&configurator)
	}

	if err := configurator.validate(); err != nil {
		return nil, err
	}

	grpcService, err := configurator.buildService()
	if err != nil {
		return nil, err
	}

	reflection.Register(grpcService)

	srv := &Service{
		grpcServer:     grpcService,
		listenEndpoint: configurator.listenEndpoint,
	}

	srv.bindImplementations(env)

	return srv, nil
}

func (s *Service) Run(ctx context.Context) error {
	listener, err := net.Listen("tcp", s.listenEndpoint)
	if err != nil {
		return err
	}

	scoppedLogger := ctxtools.Logger(ctx)
	group, ctx := errgroup.WithContext(ctx)

	group.Go(func() error {
		return s.grpcServer.Serve(listener)
	})

	group.Go(func() error {
		<-ctx.Done()
		scoppedLogger.Info("shutting down the grpc server", log.Error(ctx.Err()))

		s.grpcServer.GracefulStop()
		return nil
	})

	return group.Wait()
}

func (s *Service) bindImplementations(env *env.Env) {
	newLicenseCheckService(env).bind(s.grpcServer)
}
