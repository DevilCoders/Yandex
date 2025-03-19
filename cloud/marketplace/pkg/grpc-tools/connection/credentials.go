package connection

import (
	"context"
	"fmt"

	"a.yandex-team.ru/cloud/marketplace/pkg/errtools"
)

type Authenticator interface {
	Token(context.Context) (string, error)
}

type defaultTokenBasedAuthenticator struct {
	Authenticator
}

func newTokenBaseAuthenticator(authenticator Authenticator) *defaultTokenBasedAuthenticator {
	return &defaultTokenBasedAuthenticator{
		Authenticator: authenticator,
	}
}

func (t *defaultTokenBasedAuthenticator) GetRequestMetadata(ctx context.Context, uri ...string) (map[string]string, error) {
	token, err := t.Authenticator.Token(ctx)
	if err != nil {
		return nil, errtools.WithCurCaller(err)
	}

	return map[string]string{
		"authorization": fmt.Sprintf("Bearer %s", token),
	}, nil
}

func (t *defaultTokenBasedAuthenticator) RequireTransportSecurity() bool {
	return false
}
