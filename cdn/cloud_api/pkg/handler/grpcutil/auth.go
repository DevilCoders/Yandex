package grpcutil

import (
	"context"
	"fmt"
	"strings"

	"google.golang.org/grpc/metadata"

	"a.yandex-team.ru/cdn/cloud_api/pkg/service/auth"
	cloudauth "a.yandex-team.ru/cloud/iam/accessservice/client/iam-access-service-client-go/v1"
)

const bearerPrefix = "bearer "

type AuthClient interface {
	Authorize(ctx context.Context, permission auth.Permission, resource auth.AuthorizeResource) error
}

type MockAuthClient struct{}

func (a *MockAuthClient) Authorize(_ context.Context, _ auth.Permission, _ auth.AuthorizeResource) error {
	return nil
}

type AuthClientImpl struct {
	Service auth.AuthenticationService
}

func (c *AuthClientImpl) Authorize(ctx context.Context, permission auth.Permission, resource auth.AuthorizeResource) error {
	token, err := extractIAMToken(ctx)
	if err != nil {
		return fmt.Errorf("extact token: %w", err)
	}

	_, err = c.Service.Authorize(ctx, token, permission, resource)
	if err != nil {
		return fmt.Errorf("authorize: %w", err)
	}

	return nil
}

func extractIAMToken(ctx context.Context) (cloudauth.IAMToken, error) {
	meta, ok := metadata.FromIncomingContext(ctx)
	if !ok {
		return cloudauth.IAMToken{Token: ""}, fmt.Errorf("extract meta from context")
	}

	authorizations := meta["authorization"]
	if len(authorizations) == 0 {
		return cloudauth.IAMToken{Token: ""}, fmt.Errorf("authorization data is missing")
	}
	authorization := authorizations[0]

	if len(authorization) < len(bearerPrefix) || strings.ToLower(authorization[:len(bearerPrefix)]) != bearerPrefix {
		return cloudauth.IAMToken{Token: ""}, fmt.Errorf("authorization data invalid")
	}

	token := authorization[len(bearerPrefix):]
	iamToken := cloudauth.NewIAMToken(token)

	return iamToken, nil
}
