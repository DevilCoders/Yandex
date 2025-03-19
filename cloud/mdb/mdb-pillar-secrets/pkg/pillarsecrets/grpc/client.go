package grpc

import (
	"context"

	"google.golang.org/grpc"
	grpchealth "google.golang.org/grpc/health/grpc_health_v1"

	"a.yandex-team.ru/cloud/mdb/internal/auth/grpcauth"
	"a.yandex-team.ru/cloud/mdb/internal/auth/grpcauth/iamauth"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil/grpcerr"
	"a.yandex-team.ru/cloud/mdb/internal/secret"
	"a.yandex-team.ru/cloud/mdb/mdb-pillar-secrets/pkg/pillarsecrets"
	apiv1 "a.yandex-team.ru/cloud/mdb/mdb-pillar-secrets/proto/mdb/pillarsecrets/v1"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type Client struct {
	lg log.Logger

	conn   *grpc.ClientConn
	health grpchealth.HealthClient
}

var _ pillarsecrets.PillarSecretsClient = &Client{}

// New constructs new mdb-internal-api client
func New(ctx context.Context, target, userAgent string, cfg grpcutil.ClientConfig, l log.Logger) (*Client, error) {
	conn, err := grpcutil.NewConn(ctx, target, userAgent, cfg, l, grpcutil.WithClientCredentials(grpcauth.NewContextCredentials(iamauth.NewIAMAuthTokenModel())))
	if err != nil {
		return nil, xerrors.Errorf("dial MDB Pillar Secrets API at %q: %w", target, err)
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

func (c *Client) GetClusterPillarSecret(ctx context.Context, cid string, path []string) (secret.String, error) {
	client := apiv1.NewPillarSecretServiceClient(c.conn)
	response, err := client.GetPillarSecret(ctx, &apiv1.GetPillarSecretRequest{
		TargetClusterID: cid,
		Path:            &apiv1.GetPillarSecretRequest_PathSpec{Keys: path},
		SourcePillar:    &apiv1.GetPillarSecretRequest_ClusterID{ClusterID: cid},
	})
	if err != nil {
		return secret.String{}, grpcerr.SemanticErrorFromGRPC(err)
	}

	return secret.NewString(response.GetValue()), nil
}

func (c *Client) GetSubClusterPillarSecret(ctx context.Context, cid, subcid string, path []string) (secret.String, error) {
	client := apiv1.NewPillarSecretServiceClient(c.conn)
	response, err := client.GetPillarSecret(ctx, &apiv1.GetPillarSecretRequest{
		TargetClusterID: cid,
		Path:            &apiv1.GetPillarSecretRequest_PathSpec{Keys: path},
		SourcePillar:    &apiv1.GetPillarSecretRequest_SubClusterID{SubClusterID: subcid},
	})
	if err != nil {
		return secret.String{}, grpcerr.SemanticErrorFromGRPC(err)
	}

	return secret.NewString(response.GetValue()), nil
}

func (c *Client) GetShardPillarSecret(ctx context.Context, cid, shardID string, path []string) (secret.String, error) {
	client := apiv1.NewPillarSecretServiceClient(c.conn)
	response, err := client.GetPillarSecret(ctx, &apiv1.GetPillarSecretRequest{
		TargetClusterID: cid,
		Path:            &apiv1.GetPillarSecretRequest_PathSpec{Keys: path},
		SourcePillar:    &apiv1.GetPillarSecretRequest_ShardID{ShardID: shardID},
	})
	if err != nil {
		return secret.String{}, grpcerr.SemanticErrorFromGRPC(err)
	}

	return secret.NewString(response.GetValue()), nil
}

func (c *Client) GetHostPillarSecret(ctx context.Context, cid, fqdn string, path []string) (secret.String, error) {
	client := apiv1.NewPillarSecretServiceClient(c.conn)
	response, err := client.GetPillarSecret(ctx, &apiv1.GetPillarSecretRequest{
		TargetClusterID: cid,
		Path:            &apiv1.GetPillarSecretRequest_PathSpec{Keys: path},
		SourcePillar:    &apiv1.GetPillarSecretRequest_FQDN{FQDN: fqdn},
	})
	if err != nil {
		return secret.String{}, grpcerr.SemanticErrorFromGRPC(err)
	}

	return secret.NewString(response.GetValue()), nil
}
