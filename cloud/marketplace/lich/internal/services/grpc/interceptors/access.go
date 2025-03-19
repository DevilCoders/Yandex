package interceptors

import (
	"context"
	"time"

	"google.golang.org/grpc"
	"google.golang.org/grpc/status"

	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
)

func NewAccessLogger(logger log.Logger) grpc.UnaryServerInterceptor {
	return func(
		ctx context.Context, req interface{}, info *grpc.UnaryServerInfo, handler grpc.UnaryHandler,
	) (interface{}, error) {

		start := time.Now()

		resp, err := handler(ctx, req)

		ctxlog.Info(ctx, logger, "request done",
			log.String("method", info.FullMethod),
			log.String("code", status.Code(err).String()),
			log.Duration("request_time", time.Since(start)),
			log.Error(err),
		)

		return resp, err
	}
}
