package tracing

//
// TODO: Add opentelemetry support after arcadia packages sync sync upstream.
//

import (
	"context"
	"sync/atomic"

	"github.com/opentracing/opentracing-go"

	"a.yandex-team.ru/cloud/marketplace/pkg/ctxtools"
)

var defaultTracer tracer

func Tracer() opentracing.Tracer {
	return defaultTracer.get()
}

func SetTracer(tracer opentracing.Tracer) {
	defaultTracer.syncTracer.Store(tracer)
}

type tracer struct {
	syncTracer atomic.Value
}

func (t *tracer) get() opentracing.Tracer {
	if t := defaultTracer.syncTracer.Load(); t != nil {
		return t.(opentracing.Tracer)
	}

	return opentracing.NoopTracer{}
}

func Start(ctx context.Context, operationName string, opts ...opentracing.StartSpanOption) (opentracing.Span, context.Context) {
	requestID := ctxtools.GetRequestIDOrEmpty(ctx)
	if requestID != "" {
		opts = append(opts, opentracing.Tag{
			Key:   "request_id",
			Value: requestID,
		})
	}

	return opentracing.StartSpanFromContextWithTracer(
		ctx,
		defaultTracer.get(),
		operationName,
		opts...,
	)
}
