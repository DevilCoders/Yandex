package authentication

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/internal/ready"
)

type Result interface {
	IsAuthenticated() bool
	ServiceID() uint32
}

type Authenticator interface {
	ready.Checker
	Auth(ctx context.Context, credentials interface{}) (Result, error)
}
