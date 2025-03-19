package accessservice

import (
	"context"

	"a.yandex-team.ru/cloud/iam/accessservice/client/go/cloudauth"
	authcontext "a.yandex-team.ru/cloud/mdb/internal/auth"
	as "a.yandex-team.ru/cloud/mdb/internal/compute/accessservice"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/auth"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
)

type Authenticator struct {
	l       log.Logger
	mdb     metadb.Backend
	accserv as.AccessService
}

var _ auth.Authenticator = &Authenticator{}

func NewAuthenticator(mdb metadb.Backend, accserv as.AccessService, l log.Logger) *Authenticator {
	return &Authenticator{
		l:       l,
		mdb:     mdb,
		accserv: accserv,
	}
}

func (a *Authenticator) Authenticate(ctx context.Context, perms models.Permission, resources ...as.Resource) (as.Subject, error) {
	token, ok := authcontext.TokenFromContext(ctx)
	if !ok {
		return as.Subject{}, semerr.Authentication("missing auth token")
	}

	res, err := a.accserv.Auth(ctx, token, perms.Name, resources...)
	if err != nil {
		ctxlog.Warn(ctx, a.l, "auth request failed", log.Error(err))
		err = semerr.WhitelistErrors(err, semerr.SemanticUnavailable, semerr.SemanticAuthentication, semerr.SemanticAuthorization)
	}

	return res, err
}

func (a *Authenticator) Authorize(ctx context.Context, subject cloudauth.Subject, permission models.Permission, resources ...as.Resource) error {
	err := a.accserv.Authorize(ctx, subject, permission.Name, resources...)
	if err != nil {
		ctxlog.Warn(ctx, a.l, "authorize request failed", log.Error(err))
		err = semerr.WhitelistErrors(err, semerr.SemanticUnavailable, semerr.SemanticAuthentication, semerr.SemanticAuthorization)
	}

	return err
}
