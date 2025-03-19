package interceptors

import (
	"context"
	"time"

	"google.golang.org/grpc"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/status"

	"a.yandex-team.ru/library/go/core/log"

	"a.yandex-team.ru/cloud/marketplace/lich/internal/metrics"
	"a.yandex-team.ru/cloud/marketplace/pkg/ctxtools"
)

func NewMethodsMonitoring(h *metrics.Hub) grpc.UnaryServerInterceptor {
	return func(
		ctx context.Context, request interface{}, info *grpc.UnaryServerInfo, handler grpc.UnaryHandler,
	) (response interface{}, err error) {
		start := time.Now()
		response, err = handler(ctx, request)
		span := time.Since(start)

		scoppedLogger := ctxtools.LoggerWith(ctx, log.String("method_name", info.FullMethod))

		if h == nil {
			scoppedLogger.Debug("monitoring metrics hub was not installed")
			return
		}

		if status, ok := status.FromError(err); ok {
			h.CommitGRPCRequest(ctx, info.FullMethod, status.Code(), span)
			return
		}

		returnCode := codes.OK
		if err != nil {
			scoppedLogger.Error("unclassified error", log.Error(err))
			returnCode = codes.Internal
		}

		h.CommitGRPCRequest(ctx, info.FullMethod, returnCode, span)
		return
	}
}
