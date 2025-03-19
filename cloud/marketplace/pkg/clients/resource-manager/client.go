package rm

import (
	"context"
	"time"

	"google.golang.org/grpc"

	"a.yandex-team.ru/library/go/core/log"

	"a.yandex-team.ru/cloud/marketplace/pkg/auth"
	"a.yandex-team.ru/cloud/marketplace/pkg/grpc-tools/connection"
	"a.yandex-team.ru/cloud/marketplace/pkg/tracing"

	resources "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/resourcemanager/v1"
)

type Client struct {
	cloudService resources.CloudServiceClient

	connection *grpc.ClientConn
}

type Config struct {
	Endpoint string

	CAPath string

	InitTimeout time.Duration

	// DebugMode connect in debug mode - disabling some of some secure connections options.
	DebugMode bool
}

var DefaultConfig = Config{
	InitTimeout: 20 * time.Second,
}

func NewClient(config Config) (*Client, error) {
	return newClient(config)
}

func NewClientWithAuthProvider(config Config, authenticator connection.Authenticator, logger log.Logger) (*Client, error) {
	return newClient(config,
		connection.WithTLS(config.CAPath),
		connection.WithPerRPCAuth(authenticator),
		connection.WithInitTimeout(config.InitTimeout),
	)
}

func NewClientWithYCDefaultAuth(ctx context.Context, config Config, logger log.Logger) (*Client, error) {
	return newClient(config,
		connection.WithTLS(config.CAPath),
		connection.WithInitTimeout(config.InitTimeout),
		connection.WithPerRPCAuth(
			auth.NewYCDefaultTokenAuthenticator(ctx),
		),
	)
}

func NewClientWithAuthToken(config Config, token string) (*Client, error) {
	authenticator := auth.NewTokenAuthenticator(token)

	return newClient(config,
		connection.WithTLS(config.CAPath),
		connection.WithPerRPCAuth(&authenticator),
		connection.WithInitTimeout(config.InitTimeout),
	)
}

func (c *Client) Close() error {
	return c.connection.Close()
}

func newClient(config Config, options ...connection.Option) (*Client, error) {
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
		cloudService: resources.NewCloudServiceClient(connection),

		connection: connection,
	}

	return client, nil
}

func defaultInitContext(timeout time.Duration) (context.Context, context.CancelFunc) {
	return context.WithTimeout(context.Background(), timeout)
}
