package grpc

import (
	"context"
	"time"

	"google.golang.org/grpc/credentials"

	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/iam/v1"
	intiam "a.yandex-team.ru/cloud/mdb/internal/compute/iam"
	"a.yandex-team.ru/cloud/mdb/internal/compute/iam/internal"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

var _ intiam.TokenService = &TokenServiceClient{}

type TokenServiceClient struct {
	tokenAPI iam.IamTokenServiceClient
}

func NewTokenServiceClient(ctx context.Context, target, userAgent string, cfg grpcutil.ClientConfig, creds credentials.PerRPCCredentials, l log.Logger) (*TokenServiceClient, error) {
	conn, err := grpcutil.NewConn(ctx, target, userAgent, cfg, l, grpcutil.WithClientCredentials(creds))
	if err != nil {
		return nil, xerrors.Errorf("connecting to token API at %q: %w", target, err)
	}

	return &TokenServiceClient{
		tokenAPI: iam.NewIamTokenServiceClient(conn),
	}, nil
}

func (c *TokenServiceClient) TokenFromOauth(ctx context.Context, oauth string) (intiam.Token, error) {
	resp, err := c.tokenAPI.Create(ctx, &iam.CreateIamTokenRequest{
		Identity: &iam.CreateIamTokenRequest_YandexPassportOauthToken{YandexPassportOauthToken: oauth},
	})
	if err != nil {
		return intiam.Token{}, xerrors.Errorf("create iam token: %w", err)
	}
	return intiam.Token{
		Value:     resp.IamToken,
		ExpiresAt: time.Unix(resp.ExpiresAt.Seconds, int64(resp.ExpiresAt.Nanos)),
	}, nil
}

func (c *TokenServiceClient) ServiceAccountToken(ctx context.Context, serviceAccount intiam.ServiceAccount) (intiam.Token, error) {
	jwt, err := internal.JWTFromServiceAccount(serviceAccount)
	if err != nil {
		return intiam.Token{}, xerrors.Errorf("jwt create: %w", err)
	}
	resp, err := c.tokenAPI.Create(ctx, &iam.CreateIamTokenRequest{
		Identity: &iam.CreateIamTokenRequest_Jwt{Jwt: jwt},
	})
	if err != nil {
		return intiam.Token{}, xerrors.Errorf("create iam token: %w", err)
	}
	return intiam.Token{
		Value:     resp.IamToken,
		ExpiresAt: time.Unix(resp.ExpiresAt.Seconds, int64(resp.ExpiresAt.Nanos)),
	}, nil
}
