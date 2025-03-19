package grpc

import (
	"context"
	"net"

	"golang.org/x/sync/errgroup"
	"google.golang.org/grpc"
	"google.golang.org/grpc/reflection"

	"a.yandex-team.ru/cloud/marketplace/license_server/internal/services/env"
	console_instance_service "a.yandex-team.ru/cloud/marketplace/license_server/internal/services/grpc/license/console/instance_service"
	console_lock_service "a.yandex-team.ru/cloud/marketplace/license_server/internal/services/grpc/license/console/lock_service"
	partners_template_service "a.yandex-team.ru/cloud/marketplace/license_server/internal/services/grpc/license/partners/template_service"
	partners_template_version_service "a.yandex-team.ru/cloud/marketplace/license_server/internal/services/grpc/license/partners/template_version_service"
	priv_instance_service "a.yandex-team.ru/cloud/marketplace/license_server/internal/services/grpc/license/priv/instance_service"
	priv_lock_service "a.yandex-team.ru/cloud/marketplace/license_server/internal/services/grpc/license/priv/lock_service"
	priv_template_service "a.yandex-team.ru/cloud/marketplace/license_server/internal/services/grpc/license/priv/template_service"
	priv_template_version_service "a.yandex-team.ru/cloud/marketplace/license_server/internal/services/grpc/license/priv/template_version_service"
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
		scoppedLogger.Debug("running grpc service", log.String("endpoint", s.listenEndpoint))
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
	// private
	priv_instance_service.NewInstanceService(env).Bind(s.grpcServer)
	priv_lock_service.NewLockService(env).Bind(s.grpcServer)
	priv_template_service.NewTemplateService(env).Bind(s.grpcServer)
	priv_template_version_service.NewTemplateVersionService(env).Bind(s.grpcServer)

	// partners
	partners_template_service.NewTemplateService(env).Bind(s.grpcServer)
	partners_template_version_service.NewTemplateVersionService(env).Bind(s.grpcServer)

	// console
	console_instance_service.NewInstanceService(env).Bind(s.grpcServer)
	console_lock_service.NewLockService(env).Bind(s.grpcServer)
}
