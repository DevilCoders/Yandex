package grpcauth

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/internal/auth"
)

type ContextCredentials struct {
	tokenModel GRPCAuthTokenModel
}

func NewContextCredentials(tokenModel GRPCAuthTokenModel) ContextCredentials {
	return ContextCredentials{tokenModel: tokenModel}
}

func (c ContextCredentials) GetRequestMetadata(ctx context.Context, uri ...string) (map[string]string, error) {
	token, ok := auth.TokenFromContext(ctx)
	if !ok {
		return nil, nil
	}

	return c.tokenModel.NewAuthHeader(token), nil
}

func (ContextCredentials) RequireTransportSecurity() bool {
	return false
}
