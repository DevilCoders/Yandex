package server

import (
	"context"
	"errors"
	"fmt"
	"strings"

	"google.golang.org/grpc/metadata"

	as "a.yandex-team.ru/cloud/mdb/internal/compute/accessservice"
)

const (
	authMetadataKey = "authorization"
	tokenPrefix     = "Bearer "
)

// Auth stores auth params with client
type Auth struct {
	Client     as.AccessService
	FolderID   as.Resource
	Permission string
}

// ParseGRPCAuthToken retrieves auth token from gRPC context
func ParseGRPCAuthToken(ctx context.Context) (string, error) {
	md, ok := metadata.FromIncomingContext(ctx)
	if !ok {
		return "", errors.New("no metadata found")
	}

	token := md.Get(authMetadataKey)
	if len(token) != 1 {
		return "", errors.New("could not retrieve auth key from metadata")
	}

	if !strings.HasPrefix(token[0], tokenPrefix) {
		return "", fmt.Errorf("auth header doesn't have %q prefix: %.7s", tokenPrefix, token)
	}

	return token[0][len(tokenPrefix):], nil
}
