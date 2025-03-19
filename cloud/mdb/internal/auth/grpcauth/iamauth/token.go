package iamauth

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/internal/auth/grpcauth"
)

const (
	authMetadataKey = "authorization"
	tokenPrefix     = "Bearer "
)

func NewIAMAuthTokenModel() grpcauth.GRPCAuthTokenModel {
	return grpcauth.NewGRPCAuthTokenModel(authMetadataKey, tokenPrefix)
}

// ParseGRPCAuthToken retrieves auth token from gRPC context
func ParseGRPCAuthToken(ctx context.Context) (string, error) {
	return NewIAMAuthTokenModel().ParseGRPCAuthToken(ctx)
}
