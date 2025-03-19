package auth

import (
	"context"

	"google.golang.org/grpc/credentials"
)

type GRPCAuthenticator interface {
	credentials.PerRPCCredentials
}

type YDBAuthenticator interface {
	Token(ctx context.Context) (string, error)
}

type TokenGetter interface {
	GetToken() (string, error)
}
