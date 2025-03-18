package grpc

import (
	"context"
	"sync"
	"time"

	"google.golang.org/grpc"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/status"

	"a.yandex-team.ru/cdn/cloud_api/pkg/application/xmiddleware"
	xmetrics "a.yandex-team.ru/cdn/cloud_api/pkg/application/xmiddleware/metrics"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/metrics"
	"a.yandex-team.ru/library/go/core/metrics/solomon"
)

type metricMiddleware struct {
	logger           log.Logger
	registry         metrics.Registry
	operationMetrics sync.Map
	solomonRated     bool
	durationBuckets  metrics.DurationBuckets
}

func NewMetricMiddleware(logger log.Logger, registry metrics.Registry, opts ...MetricOption) *metricMiddleware {
	m := &metricMiddleware{
		logger:           logger,
		registry:         registry,
		operationMetrics: sync.Map{},
		solomonRated:     true,
		durationBuckets:  xmetrics.DefaultDurationBuckets,
	}

	for _, o := range opts {
		o(m)
	}

	return m
}

type MetricOption func(*metricMiddleware)

func DisableSolomonRated() func(*metricMiddleware) {
	return func(middleware *metricMiddleware) {
		middleware.solomonRated = false
	}
}

func WithDurationBuckets(buckets metrics.DurationBuckets) func(*metricMiddleware) {
	return func(middleware *metricMiddleware) {
		middleware.durationBuckets = buckets
	}
}

func (m *metricMiddleware) getMetric(key string) *requestMetric {
	value, _ := m.operationMetrics.LoadOrStore(key, &requestMetric{})
	metric := value.(*requestMetric)

	m.register(metric, key)

	return metric
}

// exhaustivestruct:ignore
type requestMetric struct {
	registerOnce sync.Once

	requestCount     metrics.Counter
	requestDuration  metrics.Timer
	errorsCount      metrics.Counter
	panicsCount      metrics.Counter
	inflightRequests metrics.Gauge
	requestGRPCCodes metrics.CounterVec
}

const (
	endpoint = "endpoint"
	grpcCode = "grpc_code"
)

func (m *metricMiddleware) register(metric *requestMetric, key string) {
	metric.registerOnce.Do(func() {
		r := m.registry.WithTags(map[string]string{
			endpoint: key,
		})

		metric.requestCount = r.Counter("request_count")
		metric.requestDuration = r.DurationHistogram("request_duration", m.durationBuckets)
		metric.errorsCount = r.Counter("err_count")
		metric.panicsCount = r.Counter("panics_count")
		metric.inflightRequests = r.Gauge("inflight_requests")
		metric.requestGRPCCodes = r.CounterVec("grpc_code_count", []string{grpcCode})

		if m.solomonRated {
			solomon.Rated(metric.requestCount)
			solomon.Rated(metric.requestDuration)
			solomon.Rated(metric.errorsCount)
			solomon.Rated(metric.panicsCount)
			solomon.Rated(metric.requestGRPCCodes)
		}
	})
}

func (m *metricMiddleware) RequestStat() func(ctx context.Context, req interface{}, info *grpc.UnaryServerInfo, handler grpc.UnaryHandler) (interface{}, error) {
	return func(ctx context.Context, req interface{}, info *grpc.UnaryServerInfo, handler grpc.UnaryHandler) (_ interface{}, err error) {
		deferFunc := func(startTime time.Time, metric *requestMetric) {
			metric.requestDuration.RecordDuration(time.Since(startTime))
			metric.inflightRequests.Add(-1)

			if p := recover(); p != nil {
				metric.panicsCount.Inc()
				panic(p)
				// TODO: span error
			}
		}

		operationName := xmiddleware.GetOperationName(ctx)
		metric := m.getMetric(operationName)

		metric.requestCount.Inc()
		metric.inflightRequests.Add(1)

		startTime := time.Now()
		defer deferFunc(startTime, metric)

		resp, err := handler(ctx, req)
		// TODO: if err set to span

		code := status.Code(err)
		metric.requestGRPCCodes.With(withGRPCCode(code)).Inc()

		return resp, err
	}
}

func withGRPCCode(code codes.Code) map[string]string {
	return map[string]string{
		grpcCode: code.String(),
	}
}
