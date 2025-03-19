package auth

import (
	"context"
)

//go:generate ../../../scripts/mockgen.sh Authenticator

type Authenticator interface {
	Authenticate(ctx context.Context, configTypes []string) error
}
