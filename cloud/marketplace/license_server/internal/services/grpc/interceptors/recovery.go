package interceptors

import (
	"context"

	grpc_recovery "github.com/grpc-ecosystem/go-grpc-middleware/recovery"
	"google.golang.org/grpc"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/status"

	"a.yandex-team.ru/cloud/marketplace/license_server/pkg/panictools"
	"a.yandex-team.ru/library/go/core/log"
)

// MakeDefaultRecoveryFunc create default recovery interceptor which log error to
// provided logger and return codes.Internal to client.
func MakeDefaultRecoveryFunc(logger log.Logger) grpc_recovery.RecoveryHandlerFuncContext {
	return func(ctx context.Context, p interface{}) error {
		panictools.LogRecovery(ctx, logger, p)
		return status.Error(codes.Internal, "internal error")
	}
}

// NewRecovery constructs unary recovery interceptor.
func NewRecovery(recoveryHandler grpc_recovery.RecoveryHandlerFuncContext) grpc.UnaryServerInterceptor {
	return grpc_recovery.UnaryServerInterceptor(
		grpc_recovery.WithRecoveryHandlerContext(recoveryHandler),
	)
}

// NewDefaultRecovery constructs default recovery interceptor.
func NewDefaultRecovery(logger log.Logger) grpc.UnaryServerInterceptor {
	return NewRecovery(MakeDefaultRecoveryFunc(logger))
}
