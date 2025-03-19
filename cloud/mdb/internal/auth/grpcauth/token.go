package grpcauth

import (
	"context"
	"strings"

	"google.golang.org/grpc/metadata"

	"a.yandex-team.ru/cloud/mdb/internal/semerr"
)

type GRPCAuthTokenModel struct {
	authMetadataKey string
	tokenPrefix     string
}

func NewGRPCAuthTokenModel(authMetadataKey, tokenPrefix string) GRPCAuthTokenModel {
	return GRPCAuthTokenModel{
		authMetadataKey: authMetadataKey,
		tokenPrefix:     tokenPrefix,
	}
}

// ParseGRPCAuthToken retrieves auth token from gRPC context
func (p GRPCAuthTokenModel) ParseGRPCAuthToken(ctx context.Context) (string, error) {
	md, ok := metadata.FromIncomingContext(ctx)
	if !ok {
		return "", semerr.Authentication("no metadata found")
	}

	token := md.Get(p.authMetadataKey)
	if len(token) != 1 {
		return "", semerr.Authentication("could not retrieve auth key from metadata")
	}

	if !strings.HasPrefix(token[0], p.tokenPrefix) {
		return "", semerr.Authenticationf("auth header doesn't have %q prefix: %.7s", p.tokenPrefix, token)
	}

	return token[0][len(p.tokenPrefix):], nil
}

// NewAuthHeader returns authentication data in a format suitable for gRPC metadata
func (p GRPCAuthTokenModel) NewAuthHeader(token string) map[string]string {
	return map[string]string{p.authMetadataKey: p.tokenPrefix + token}
}
