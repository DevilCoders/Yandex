package interceptors

import (
	"context"

	"google.golang.org/grpc"

	"a.yandex-team.ru/cloud/mdb/internal/grpcutil/grpcerr"
	"a.yandex-team.ru/library/go/core/log"
)

func newUnaryServerInterceptorErrorToGRPC(exposeDebug bool, l log.Logger) grpc.UnaryServerInterceptor {
	return func(ctx context.Context, req interface{}, info *grpc.UnaryServerInfo, handler grpc.UnaryHandler) (interface{}, error) {
		resp, err := handler(ctx, req)
		if err != nil {
			st := grpcerr.ErrorToGRPC(err, exposeDebug, l)
			return resp, st.Err()
		}
		return resp, err
	}
}

func newStreamServerInterceptorErrorToGRPC(exposeDebug bool, l log.Logger) grpc.StreamServerInterceptor {
	return func(srv interface{}, ss grpc.ServerStream, info *grpc.StreamServerInfo, handler grpc.StreamHandler) error {
		if err := handler(srv, ss); err != nil {
			st := grpcerr.ErrorToGRPC(err, exposeDebug, l)
			return st.Err()
		}
		return nil
	}
}
