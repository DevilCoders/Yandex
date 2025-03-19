package grpc

import (
	"context"

	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials"
	grpchealth "google.golang.org/grpc/health/grpc_health_v1"

	chv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/clickhouse/v1"
	pgv1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/postgresql/v1"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/pkg/internalapi"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type Client struct {
	lg log.Logger

	conn   *grpc.ClientConn
	health grpchealth.HealthClient
}

var _ internalapi.Client = &Client{}

// New constructs new mdb-internal-api client
func New(ctx context.Context, target, userAgent string, cfg grpcutil.ClientConfig, creds credentials.PerRPCCredentials, l log.Logger) (*Client, error) {
	conn, err := grpcutil.NewConn(ctx, target, userAgent, cfg, l, grpcutil.WithClientCredentials(creds))
	if err != nil {
		return nil, xerrors.Errorf("dial MDB Internal API at %q: %w", target, err)
	}

	return &Client{
		lg:     l,
		conn:   conn,
		health: grpchealth.NewHealthClient(conn),
	}, nil
}

func (c *Client) Conn() *grpc.ClientConn {
	return c.conn
}

func (c *Client) IsReady(ctx context.Context) error {
	resp, err := c.health.Check(ctx, &grpchealth.HealthCheckRequest{})
	if err != nil {
		return err
	}

	switch resp.Status {
	case grpchealth.HealthCheckResponse_SERVING:
		return nil
	case grpchealth.HealthCheckResponse_NOT_SERVING:
		return xerrors.New("service is not serving")
	case grpchealth.HealthCheckResponse_SERVICE_UNKNOWN:
		return xerrors.New("requested health info for unknown service")
	}

	return xerrors.Errorf("unknown response for health request: %d", resp.Status)
}

func (c *Client) PostgreSQL() internalapi.PostgreSQLClient {
	return &pgClient{
		c:        c,
		cluster:  pgv1.NewClusterServiceClient(c.conn),
		database: pgv1.NewDatabaseServiceClient(c.conn),
	}
}

func (c *Client) ClickHouse() internalapi.ClickHouseClient {
	return &chClient{
		c:        c,
		cluster:  chv1.NewClusterServiceClient(c.conn),
		database: chv1.NewDatabaseServiceClient(c.conn),
	}
}
