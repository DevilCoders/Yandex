package tracing

import (
	"context"
	"io"
	"runtime"
	"strconv"
	"strings"
	"time"

	"github.com/jonboulle/clockwork"
	"github.com/opentracing/opentracing-go"
	"github.com/opentracing/opentracing-go/ext"
	otlog "github.com/opentracing/opentracing-go/log"
	"github.com/uber/jaeger-client-go"

	"a.yandex-team.ru/cloud/compute/go-common/pkg/metriclabels"
	"a.yandex-team.ru/cloud/compute/go-common/pkg/xrequestid"
)

var noopTracer = opentracing.NoopTracer{}

type SpanRateLimiter struct {
	LastSpan        time.Time
	InitialInterval time.Duration
	MaxInterval     time.Duration

	interval time.Duration
	clock    clockwork.Clock
}

func (sl *SpanRateLimiter) StartSpanFromContext(ctx context.Context, operationName string, opts ...opentracing.StartSpanOption) (opentracing.Span, context.Context) {
	if span := opentracing.SpanFromContext(ctx); span == nil || span.Tracer() == noopTracer {
		return noopTracer.StartSpan(operationName), ctx
	}

	if sl.clock == nil {
		sl.clock = clockwork.NewRealClock()
	}

	if sl.clock.Since(sl.LastSpan) < sl.interval {
		return DisableSpans(ctx)
	}

	if sl.interval == 0 {
		sl.interval = sl.InitialInterval
	} else {
		sl.interval *= 2
	}

	if sl.MaxInterval == 0 {
		sl.MaxInterval = time.Hour
	}
	if sl.interval > sl.MaxInterval {
		sl.interval = sl.MaxInterval
	}

	sl.LastSpan = sl.clock.Now()
	return StartSpanFromContext(ctx, operationName, opts...)
}

// log error and finish span with error tag
func Finish(span opentracing.Span, err error) {
	if span == nil {
		return
	}

	if err != nil {
		ext.Error.Set(span, true)
		WithError(span, err)
	}
	span.Finish()
}

func DisableSpans(ctx context.Context) (opentracing.Span, context.Context) {
	span := noopTracer.StartSpan("")
	return span, opentracing.ContextWithSpan(ctx, span)
}

func FollowSpanFromContext(ctx context.Context, operationName string, opts ...opentracing.StartSpanOption) (opentracing.Span, context.Context) {
	if !tracingEnabled(ctx) {
		return noopTracer.StartSpan(operationName), ctx
	}

	span := opentracing.SpanFromContext(ctx)
	span, ctx = opentracing.StartSpanFromContext(ctx, operationName, opentracing.FollowsFrom(span.Context()))
	return span, ctx
}

func InitSpan(ctx context.Context, operationName string, opts ...opentracing.StartSpanOption) (opentracing.Span, context.Context) {
	span, ctx := opentracing.StartSpanFromContext(ctx, operationName, opts...)
	labels := metriclabels.Get(ctx)
	if labels.TaskID != "" {
		span.SetTag("task_id", labels.TaskID)
	}
	if labels.ComputeTaskID != "" {
		span.SetTag("compute_task_id", labels.ComputeTaskID)
	}
	if labels.OperationID != "" {
		span.SetTag("operation_id", labels.OperationID)
	}
	if requestID, ok := xrequestid.FromContext(ctx); ok {
		span.SetTag("request_id", requestID)
	}
	return span, ctx
}

func InitJaegerTracing(cfg Config) (io.Closer, error) {
	if cfg.Address == "" {
		return noopCloser{}, nil
	}

	transport, err := jaeger.NewUDPTransport(cfg.Address, 0)
	if err != nil {
		return nil, err
	}

	reporters := make([]jaeger.Reporter, 0)
	reporters = append(reporters, jaeger.NewRemoteReporter(transport))
	// TODO(frystile): add custom reporters

	tracer, closer := jaeger.NewTracer(
		cfg.Service, jaeger.NewConstSampler(true),
		jaeger.NewCompositeReporter(reporters...),
		jaeger.TracerOptions.MaxTagValueLength(512),
	)
	opentracing.SetGlobalTracer(tracer)

	return closer, nil
}

// StartSpanFromContext returns new Span and Context with attaching the span. Unlike the method from opentracing-go
// it returns NoopSpan if there is no root Span and the Context is returned unchanged
func StartSpanFromContext(ctx context.Context, operationName string, opts ...opentracing.StartSpanOption) (opentracing.Span, context.Context) {
	if tracingEnabled(ctx) {
		parentSpan := opentracing.SpanFromContext(ctx)
		span := opentracing.StartSpan(operationName, append(opts, opentracing.ChildOf(parentSpan.Context()))...)
		return span, opentracing.ContextWithSpan(ctx, span)
	}
	return noopTracer.StartSpan(operationName), ctx
}

func StartSpanFromContextFunc(ctx context.Context, opts ...opentracing.StartSpanOption) (opentracing.Span, context.Context) {
	if !tracingEnabled(ctx) {
		return noopTracer.StartSpan(""), ctx
	}

	pc, file, line, ok := runtime.Caller(1)
	funcName := ""
	if ok {
		f := runtime.FuncForPC(pc)
		if f == nil {
			funcName = "anonym_" + file + ":" + strconv.Itoa(line)
		} else {
			funcName = f.Name()
			lastSlash := strings.LastIndex(funcName, "/")
			if lastSlash != -1 && lastSlash < len(funcName)-1 {
				funcName = funcName[lastSlash+1:]
			}
		}
	}
	return StartSpanFromContext(ctx, funcName, opts...)
}

// helper to log error
func WithError(span opentracing.Span, err error) {
	if span == nil {
		return
	}
	if err != nil {
		span.LogFields(
			otlog.String("event", "error"),
			otlog.String("message", err.Error()),
		)
	}
}

type traceIDKey struct{}

// WithTraceID returns new context with an attached trace ID
func WithTraceID(ctx context.Context, traceid uint64) context.Context {
	return context.WithValue(ctx, traceIDKey{}, traceid)
}

// GetTraceID returns an attached trace ID
func GetTraceID(ctx context.Context) (uint64, bool) {
	if value, ok := ctx.Value(traceIDKey{}).(uint64); ok {
		return value, ok
	}
	return 0, false
}

type noopCloser struct{}

func (noopCloser) Close() error {
	return nil
}

func tracingEnabled(ctx context.Context) bool {
	span := opentracing.SpanFromContext(ctx)
	return span != nil && span.Tracer() != noopTracer
}
