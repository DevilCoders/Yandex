package combinedauth

import (
	"context"
	"net/http"

	"a.yandex-team.ru/cloud/mdb/internal/auth/httpauth"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

var _ httpauth.Authenticator = &authenticator{}

type authenticator struct {
	auths []httpauth.Authenticator
	l     log.Logger
}

func New(l log.Logger, auths ...httpauth.Authenticator) (*authenticator, error) {
	if len(auths) == 0 {
		return nil, xerrors.Errorf("at least one Authenticator should be passed")
	}
	return &authenticator{
		auths: auths,
		l:     l,
	}, nil
}

func (a authenticator) Auth(ctx context.Context, request *http.Request) error {
	for _, auth := range a.auths {
		err := auth.Auth(ctx, request)
		switch {
		case err == nil:
			return nil
		case xerrors.Is(err, httpauth.ErrNoAuthCredentials):
			ctxlog.Debugf(ctx, a.l, "no auth creds %s", err.Error())
			continue
		default:
			return err
		}
	}

	return xerrors.Errorf("no credentials for any authenticator: %w", httpauth.ErrNoAuthCredentials)
}

func (a authenticator) Ping(ctx context.Context) error {
	for _, auth := range a.auths {
		if err := auth.Ping(ctx); err != nil {
			return err
		}
	}
	return nil
}
