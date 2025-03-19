package grpcserver

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/internal/auth/grpcauth/iamauth"
	as "a.yandex-team.ru/cloud/mdb/internal/compute/accessservice"
)

type Auth struct {
	client         as.AccessService
	folderID       as.Resource
	permission     string
	skipAuthErrors bool // TODO: remove later
}

func (s *InstanceService) authorize(ctx context.Context) (string, error) {
	token, err := iamauth.ParseGRPCAuthToken(ctx)
	if err != nil && !s.auth.skipAuthErrors {
		return "", err
	}

	subject, err := s.auth.client.Auth(ctx, token, s.auth.permission, s.auth.folderID)
	if err != nil && !s.auth.skipAuthErrors {
		return "", err
	}

	id, err := subject.ID()
	if err != nil {
		if err == as.NoIDError {
			return "anonymous", nil
		}
		return "", err
	}

	return id, nil
}

func NewAuth(client as.AccessService, folderID as.Resource, permission string, skip bool) *Auth {
	return &Auth{
		client:         client,
		folderID:       folderID,
		permission:     permission,
		skipAuthErrors: skip,
	}
}
