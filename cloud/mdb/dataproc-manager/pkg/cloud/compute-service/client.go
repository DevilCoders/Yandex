package cloud

import (
	"context"
	"sync"
	"time"

	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials"

	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/compute/v1"
	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/operation"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil/interceptors"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type ComputeServiceConfig struct {
	CAPath                string        `json:"capath" yaml:"capath"`
	ServiceURL            string        `json:"service_url" yaml:"service_url"`
	GrpcTimeout           time.Duration `json:"grpc_timeout" yaml:"grpc_timeout"`
	OperationWaitTimeout  time.Duration `json:"operation_wait_timeout" yaml:"operation_wait_timeout"`
	OperationPollInterval time.Duration `json:"operation_poll_interval" yaml:"operation_poll_interval"`
}

// DefaultConfig describes configuration for grpc client
func DefaultConfig() ComputeServiceConfig {
	return ComputeServiceConfig{
		CAPath:                "/opt/yandex/allCAs.pem",
		ServiceURL:            "compute-service-api.cloud-preprod.yandex.net:9051",
		GrpcTimeout:           30 * time.Second,
		OperationWaitTimeout:  2 * time.Minute,
		OperationPollInterval: 3 * time.Second,
	}
}

type client struct {
	logger    log.Logger
	conn      *grpc.ClientConn
	connMutex sync.Mutex
	config    ComputeServiceConfig
	useragent string
	creds     credentials.PerRPCCredentials
}

type ComputeServiceClient interface {
	Get(ctx context.Context, instanceID string) (*compute.Instance, error)
	Wait(ctx context.Context, op *operation.Operation) error
}

var _ ComputeServiceClient = &client{}

// New constructs grpc client
// It is a thread safe.
func New(config ComputeServiceConfig, creds credentials.PerRPCCredentials, logger log.Logger) ComputeServiceClient {
	client := &client{
		config:    config,
		logger:    logger,
		useragent: "Dataproc-Manager",
		creds:     creds,
	}
	return client
}

func (c *client) getConnection(ctx context.Context) (*grpc.ClientConn, error) {
	if c.conn != nil {
		return c.conn, nil
	}

	c.connMutex.Lock()
	defer c.connMutex.Unlock()

	if c.conn != nil {
		return c.conn, nil
	}

	conn, err := grpcutil.NewConn(
		ctx,
		c.config.ServiceURL,
		c.useragent,
		grpcutil.ClientConfig{
			Security: grpcutil.SecurityConfig{
				TLS: grpcutil.TLSConfig{
					CAFile: c.config.CAPath,
				},
			},
			Retries: interceptors.ClientRetryConfig{
				PerRetryTimeout: c.config.GrpcTimeout,
			},
		},
		c.logger,
		grpcutil.WithClientCredentials(c.creds),
	)
	if err != nil {
		return nil, xerrors.Errorf("can not connect to URL %q: %w", c.config.ServiceURL, err)
	}
	c.conn = conn
	return conn, nil
}

func (c *client) Get(ctx context.Context, instanceID string) (*compute.Instance, error) {
	conn, err := c.getConnection(ctx)
	if err != nil {
		return nil, err
	}
	grpcClient := compute.NewInstanceServiceClient(conn)

	var instance *compute.Instance
	instance, err = grpcClient.Get(ctx, &compute.GetInstanceRequest{InstanceId: instanceID})
	if err != nil {
		return nil, xerrors.Errorf("can not call method 'GetInstance': %w", err)
	}
	return instance, nil
}

func (c *client) Wait(ctx context.Context, op *operation.Operation) error {
	conn, err := c.getConnection(ctx)
	if err != nil {
		return err
	}
	grpcClient := compute.NewOperationServiceClient(conn)

	for {
		start := time.Now()
		op, err = grpcClient.Get(ctx, &compute.GetOperationRequest{OperationId: op.Id})
		if err != nil {
			c.logger.Debugf("error while getting operation %s: %s", op.Id, err)
		}
		if op.Done {
			if op.GetError().GetCode() != 0 {
				return xerrors.Errorf("Operation %s error: %s", op.Id, op.GetError())
			}
			return nil
		}
		t := time.Now()
		elapsed := t.Sub(start)
		timeoutSeconds := int(c.config.OperationWaitTimeout.Seconds())
		if int(elapsed.Seconds()) > timeoutSeconds {
			return xerrors.Errorf(
				"operation %s was not finished until the timeout %d was reached",
				op.Id,
				timeoutSeconds,
			)
		}
		time.Sleep(c.config.OperationPollInterval)
	}
}
