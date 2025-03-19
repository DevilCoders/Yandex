package grpc

import (
	"fmt"

	mw "github.com/grpc-ecosystem/go-grpc-middleware"
	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials"

	"a.yandex-team.ru/cloud/marketplace/license_server/internal/services/grpc/interceptors"
	"a.yandex-team.ru/cloud/marketplace/pkg/errtools"
	"a.yandex-team.ru/library/go/core/log"
)

type configurator struct {
	defaultLogger log.Logger
	accessLogger  log.Logger

	tlsConfig struct {
		credentialsPath string
		keyPath         string
	}

	listenEndpoint string
}

func (c configurator) validate() error {
	if c.defaultLogger == nil {
		return errtools.WithPrevCaller(
			fmt.Errorf("logger must be provided"),
		)
	}

	if c.accessLogger == nil {
		return errtools.WithPrevCaller(
			fmt.Errorf("access logger must be provided"),
		)
	}

	return nil
}

func (c configurator) buildService() (*grpc.Server, error) {
	unaryInterceptors := []grpc.UnaryServerInterceptor{
		interceptors.NewRequestDataInjector(),
		interceptors.NewDefaultRecovery(c.defaultLogger.WithName("recovery")),
		interceptors.NewAccessLogger(c.accessLogger),
	}

	unaryChain := mw.ChainUnaryServer(unaryInterceptors...)

	options := []grpc.ServerOption{
		grpc.UnaryInterceptor(unaryChain),
	}

	if c.tlsConfig.credentialsPath != "" {
		creds, err := credentials.NewServerTLSFromFile(c.tlsConfig.credentialsPath, c.tlsConfig.keyPath)
		if err != nil {
			return nil, err
		}

		options = append(options, grpc.Creds(creds))
	}

	return grpc.NewServer(options...), nil
}

type Option func(c *configurator)

func WithAccessLogger(logger log.Logger) Option {
	return func(c *configurator) {
		c.accessLogger = logger
	}
}

func WithLogger(logger log.Logger) Option {
	return func(c *configurator) {
		c.defaultLogger = logger
	}
}

func WithListenEndpoint(endpoint string) Option {
	return func(c *configurator) {
		c.listenEndpoint = endpoint
	}
}

func WithTLS(credentialsPath, keyPath string) Option {
	return func(c *configurator) {
		c.tlsConfig.credentialsPath = credentialsPath
		c.tlsConfig.keyPath = keyPath
	}
}
