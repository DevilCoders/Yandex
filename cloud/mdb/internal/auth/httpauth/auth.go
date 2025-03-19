package httpauth

import (
	"context"
	"net/http"

	"a.yandex-team.ru/cloud/mdb/internal/auth/blackbox"
	"a.yandex-team.ru/library/go/core/xerrors"
)

// Known auth errors
var (
	ErrAuthFailure       = xerrors.NewSentinel("failed to authenticate")
	ErrNoAuthCredentials = xerrors.NewSentinel("no auth credentials provided")
	ErrAuthNoRights      = xerrors.NewSentinel("action not authorized")
)

//go:generate ../../../scripts/mockgen.sh Authenticator

// Authenticator is responsible for request authentication
type Authenticator interface {
	Auth(ctx context.Context, request *http.Request) error
	Ping(ctx context.Context) error
}

type Blackbox interface {
	AuthUser(ctx context.Context, request *http.Request) (blackbox.UserInfo, error)
}
