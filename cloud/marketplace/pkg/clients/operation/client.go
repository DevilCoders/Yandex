package operation

import (
	"a.yandex-team.ru/cloud/bitbucket/public-api/yandex/cloud/operation"
	"context"
	"time"

	"google.golang.org/grpc"

	"a.yandex-team.ru/cloud/marketplace/pkg/auth"
	"a.yandex-team.ru/cloud/marketplace/pkg/grpc-tools/connection"
	"a.yandex-team.ru/cloud/marketplace/pkg/tracing"
	"a.yandex-team.ru/library/go/core/log"
)

type Client struct {
	operationService operation.OperationServiceClient

	connection *grpc.ClientConn

	logger         log.Logger
	OpTimeout      time.Duration
	OpPollInterval time.Duration
}

type Config struct {
	Endpoint       string
	OpTimeout      time.Duration
	OpPollInterval time.Duration
	DebugMode      bool
	CAPath         string
	InitTimeout    time.Duration
}

func NewClientWithAuthProvider(config Config, authenticator connection.Authenticator, logger log.Logger) (*Client, error) {
	return newClient(config,
		logger,
		connection.WithTLS(config.CAPath),
		connection.WithPerRPCAuth(authenticator),
		connection.WithInitTimeout(config.InitTimeout),
	)
}

func NewClientWithYCDefaultAuth(ctx context.Context, config Config, logger log.Logger) (*Client, error) {
	return newClient(config,
		logger,
		connection.WithTLS(config.CAPath),
		connection.WithInitTimeout(config.InitTimeout),
		connection.WithPerRPCAuth(
			auth.NewYCDefaultTokenAuthenticator(ctx),
		),
	)
}

func NewClientWithAuthToken(config Config, token string, logger log.Logger) (*Client, error) {
	authenticator := auth.NewTokenAuthenticator(token)

	return newClient(config,
		logger,
		connection.WithTLS(config.CAPath),
		connection.WithPerRPCAuth(&authenticator),
		connection.WithInitTimeout(config.InitTimeout),
	)
}

func (c *Client) Close() error {
	return c.connection.Close()
}

func newClient(config Config, logger log.Logger, options ...connection.Option) (*Client, error) {
	if config.DebugMode {
		options = append(options, connection.WithDebugMode())
	}

	options = append(options, connection.WithTracer(tracing.Tracer()))

	connection, err := connection.Connection(context.Background(), config.Endpoint,
		options...,
	)

	if err != nil {
		return nil, err
	}

	client := &Client{
		operationService: operation.NewOperationServiceClient(connection),
		connection:       connection,
		OpTimeout:        config.OpTimeout,
		OpPollInterval:   config.OpPollInterval,
		logger:           logger,
	}

	return client, nil
}

func defaultInitContext(timeout time.Duration) (context.Context, context.CancelFunc) {
	return context.WithTimeout(context.Background(), timeout)
}
