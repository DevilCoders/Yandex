package auth

import (
	"context"
	"errors"
	"testing"

	"github.com/stretchr/testify/assert"
	"go.uber.org/zap"
	"go.uber.org/zap/zaptest"
	"google.golang.org/grpc"

	"a.yandex-team.ru/cloud/compute/go-common/logging/ctxlog"
)

var (
	_ grpc.UnaryServerInterceptor = serverInterceptor{}.UnaryServerInterceptor
)

type accessServiceClientMock struct {
	User  string
	Error error
}

func (c accessServiceClientMock) Authorize(ctx context.Context, permission string) (string, error) {
	return c.User, c.Error
}

func TestAuthInterceptor(t *testing.T) {
	at := assert.New(t)

	logger := zaptest.NewLogger(t, zaptest.WrapOptions(zap.Development()))
	ctx := context.Background()
	ctx = ctxlog.WithLogger(ctx, logger)
	testErr := errors.New("testErr")

	handlerCalled := 0
	var okHandler = func(ctx context.Context, req interface{}) (interface{}, error) {
		handlerCalled++
		return "OK", nil
	}
	res, err := NewServerInterceptor(accessServiceClientMock{User: "user", Error: nil})(ctx, nil, nil, okHandler)
	at.Equal(1, handlerCalled)
	at.Equal("OK", res)
	at.NoError(err)

	handlerCalled = 0
	res, err = NewServerInterceptor(accessServiceClientMock{User: "", Error: testErr})(ctx, nil, nil, okHandler)
	at.Equal(0, handlerCalled)
	at.Nil(res)
	at.Equal(testErr, err)
}
