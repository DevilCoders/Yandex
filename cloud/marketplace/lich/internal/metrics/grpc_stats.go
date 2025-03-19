package metrics

import (
	"context"
	"time"

	"google.golang.org/grpc/codes"

	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/metrics/solomon"

	"a.yandex-team.ru/cloud/marketplace/pkg/ctxtools"
)

func (h *Hub) RegisterGRPCHandler(handlerName string) {
	for grpcStatus := codes.Code(0); grpcStatus <= codes.Unauthenticated; grpcStatus++ {
		code := codes.Code(grpcStatus)
		if description := code.String(); description != "" {
			key := handlerMetricsKey{
				handlerName: handlerName,
				status:      int(grpcStatus),
			}

			h.handlersMetrics[key] = h.registerGRPCMetric(handlerName, code)
		}
	}
}

func (h *Hub) registerGRPCMetric(handlerName string, grpcStatus codes.Code) *restHandlerMetrics {
	subregistry := h.registry.WithPrefix("grpc").WithTags(tags{
		"grpc_code":   grpcStatus.String(),
		"method_name": handlerName,
	})

	counter := subregistry.Counter("call_count")
	solomon.Rated(counter)

	return &restHandlerMetrics{
		counter: counter,
		timer:   subregistry.DurationHistogram("method_timings", defaultResponsesBucket),
	}
}

func (h *Hub) CommitGRPCRequest(ctx context.Context, handlerName string, code codes.Code, duration time.Duration) {
	scopedLogger := ctxtools.LoggerWith(ctx, log.String("method_name", handlerName), log.String("grpc_status", code.String()))

	metrics := h.handlersMetrics[handlerMetricsKey{handlerName, int(code)}]
	if metrics == nil {
		scopedLogger.Warn("unexpected rest handler status code")

		metrics = h.registerGRPCMetric(handlerName, code)

		h.mu.Lock()
		defer h.mu.Unlock()

		key := handlerMetricsKey{
			handlerName: handlerName,
			status:      int(code),
		}

		h.handlersMetrics[key] = metrics
	}

	metrics.counter.Inc()
	metrics.timer.RecordDuration(duration)

	scopedLogger.Debug("grpc method metrics has been committed")
}
