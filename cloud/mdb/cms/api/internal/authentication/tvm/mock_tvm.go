package tvm

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/authentication"
)

type MockAuthenticator struct {
	serviceID uint32
}

func (a *MockAuthenticator) Auth(ctx context.Context, credentials interface{}) (authentication.Result, error) {
	return NewResult(a.serviceID), nil
}

func (a *MockAuthenticator) IsReady(ctx context.Context) error {
	return nil
}

func NewMock(alwaysServiceID uint32) authentication.Authenticator {
	return &MockAuthenticator{
		serviceID: alwaysServiceID,
	}
}
