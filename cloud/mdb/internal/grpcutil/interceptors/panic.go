package interceptors

import (
	"context"
	"fmt"
	"runtime/debug"

	"google.golang.org/grpc"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/status"

	"a.yandex-team.ru/cloud/mdb/internal/sentry"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
)

func handlePanic(ctx context.Context, req interface{}, method string, p interface{}, exposeDebug bool, l log.Logger) error {
	// Do not forget to capture stack trace!
	ctxlog.Error(ctx, l, fmt.Sprintf("panicked: %+v\n%s", p, string(debug.Stack())))

	var err error
	switch pval := p.(type) {
	case nil:
		break
	case error:
		err = pval
	default:
		err = fmt.Errorf("panicked with: %+v", pval)
	}

	sentry.GlobalClient().CaptureError(ctx, err, sentryContextTags(ctx, req, method))

	msg := "unknown error"
	if exposeDebug {
		msg = err.Error()
	}

	return status.Errorf(codes.Unknown, msg)
}

func newUnaryServerInterceptorPanic(exposeDebug bool, l log.Logger) grpc.UnaryServerInterceptor {
	return func(ctx context.Context, req interface{}, info *grpc.UnaryServerInfo, handler grpc.UnaryHandler) (resp interface{}, err error) {
		defer func() {
			if r := recover(); r != nil {
				err = handlePanic(ctx, req, info.FullMethod, r, exposeDebug, l)
			}
		}()

		resp, err = handler(ctx, req)
		return resp, err
	}
}

func newStreamServerInterceptorPanic(exposeDebug bool, l log.Logger) grpc.StreamServerInterceptor {
	return func(srv interface{}, ss grpc.ServerStream, info *grpc.StreamServerInfo, handler grpc.StreamHandler) (err error) {
		defer func() {
			if r := recover(); r != nil {
				err = handlePanic(ss.Context(), nil, info.FullMethod, r, exposeDebug, l)
			}
		}()

		return handler(srv, ss)
	}
}
