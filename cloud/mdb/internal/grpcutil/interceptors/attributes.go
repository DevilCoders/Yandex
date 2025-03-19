package interceptors

import (
	"context"

	"google.golang.org/grpc"

	"a.yandex-team.ru/cloud/mdb/internal/request"
)

func newUnaryServerInterceptorRequestAttributes() grpc.UnaryServerInterceptor {
	return func(ctx context.Context, req interface{}, info *grpc.UnaryServerInfo, handler grpc.UnaryHandler) (interface{}, error) {
		ctx = request.WithAttributes(ctx)
		return handler(ctx, req)
	}
}

func newStreamServerInterceptorRequestAttributes() grpc.StreamServerInterceptor {
	return func(srv interface{}, ss grpc.ServerStream, info *grpc.StreamServerInfo, handler grpc.StreamHandler) error {
		ctx := request.WithAttributes(ss.Context())
		return handler(srv, NewServerStreamWithContext(ss, ctx))
	}
}
