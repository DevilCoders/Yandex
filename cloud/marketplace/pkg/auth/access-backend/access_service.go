package access

import (
	"context"

	"google.golang.org/grpc"

	"a.yandex-team.ru/cloud/iam/accessservice/client/go/cloudauth"

	"a.yandex-team.ru/cloud/marketplace/pkg/auth/permissions"
	"a.yandex-team.ru/cloud/marketplace/pkg/ctxtools"
	"a.yandex-team.ru/cloud/marketplace/pkg/grpc-tools/connection"
	"a.yandex-team.ru/cloud/marketplace/pkg/grpc-tools/meta"
	"a.yandex-team.ru/cloud/marketplace/pkg/tracing"
)

var BillingAdminResource = cloudauth.Resource{
	Type: "billing.account", // Name
	ID:   "admin",           // Id
}

type Client struct {
	asClient *cloudauth.AccessServiceClient

	connection *grpc.ClientConn
}

type Config struct {
	Endpoint string
	CAPath   string
}

func NewClient(initCtx context.Context, config Config) (*Client, error) {
	connection, err := connection.Connection(initCtx, config.Endpoint,
		connection.WithTLS(config.CAPath),
		connection.WithTracer(
			tracing.Tracer(),
		),
	)

	if err != nil {
		return nil, err
	}

	client := &Client{
		asClient:   cloudauth.NewAccessServiceClient(connection),
		connection: connection,
	}

	return client, nil
}

func (c *Client) AuthorizeBillingAdmin(ctx context.Context, perm permissions.Permission) error {
	token := c.acquireAuthToken(ctx)
	if token == "" {
		return ErrMissingAuthToken
	}

	return c.authorizeBillingAdmin(ctx, token, perm)
}

func (c *Client) AuthorizeBillingAdminWithToken(ctx context.Context, token string, perm permissions.Permission) error {
	return c.authorizeBillingAdmin(ctx, token, perm)
}

func (c *Client) Close() error {
	return c.connection.Close()
}

func (c *Client) authorizeBillingAdmin(ctx context.Context, token string, perm permissions.Permission) error {
	span, spanCtx := tracing.Start(ctx, "as:Authorize")
	defer span.Finish()

	_, err := c.asClient.Authorize(spanCtx, cloudauth.NewIAMToken(token), perm.String(), BillingAdminResource)
	if err := c.mapGRPCErr(spanCtx, err); err != nil {
		return err
	}

	return nil
}

func (c *Client) acquireAuthToken(ctx context.Context) string {
	token := ctxtools.GetAuthToken(ctx)
	if token == "" {
		return meta.NewMeta(ctx).AuthorizationToken()
	}

	return token
}
