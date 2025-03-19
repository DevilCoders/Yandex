package mocks

import (
	"context"

	"github.com/stretchr/testify/mock"
)

type AuthTokenProvider struct {
	mock.Mock
}

func (a *AuthTokenProvider) Token(ctx context.Context) (string, error) {
	args := a.Called()
	return args.Get(0).(string), args.Error(1)
}
