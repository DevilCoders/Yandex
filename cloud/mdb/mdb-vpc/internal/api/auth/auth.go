package auth

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/internal/auth/grpcauth/iamauth"
	as "a.yandex-team.ru/cloud/mdb/internal/compute/accessservice"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type Auth struct {
	client as.AccessService
}

func (a *Auth) Authorize(ctx context.Context, perm Permission, projectID string) (string, error) {
	token, err := iamauth.ParseGRPCAuthToken(ctx)
	if err != nil {
		return "", err
	}

	subject, err := a.client.Auth(ctx, token, string(perm), as.ResourceFolder(projectID))
	if err != nil {
		return "", err
	}

	id, err := subject.ID()
	if err != nil {
		return "", xerrors.Errorf("get ID from subject: %w", err)
	}

	return id, nil
}

func NewAuth(client as.AccessService) *Auth {
	return &Auth{client: client}
}
