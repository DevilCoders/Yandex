package tracing

import (
	"context"
	"encoding/json"
	"io"

	"github.com/opentracing/opentracing-go"
	"github.com/opentracing/opentracing-go/ext"

	"a.yandex-team.ru/library/go/core/xerrors"
)

// Tracer wraps opentracing tracer
type Tracer struct {
	opentracing.Tracer
	io.Closer
}

func SetErrorOnSpan(span opentracing.Span, err error) {
	ext.LogError(span, err)
	// TODO: additional error info? Like semantic error type if there is one...
}

// FollowSpanFromContext is equivalent of opentracing.StartSpanFromContext but makes
// created span follow parent span instead of being its child.
func FollowSpanFromContext(ctx context.Context, operationName string, opts ...opentracing.StartSpanOption) (opentracing.Span, context.Context) {
	if parentSpan := opentracing.SpanFromContext(ctx); parentSpan != nil {
		opts = append(opts, opentracing.FollowsFrom(parentSpan.Context()))
	}

	span := opentracing.StartSpan(operationName, opts...)
	return span, opentracing.ContextWithSpan(ctx, span)
}

func MarshalSpanFromContext(ctx context.Context) ([]byte, error) {
	span := opentracing.SpanFromContext(ctx)
	if span == nil {
		return nil, xerrors.New("no span found inside context")
	}

	return MarshalSpan(span)
}

func MarshalSpan(span opentracing.Span) ([]byte, error) {
	carrier := opentracing.TextMapCarrier{}
	if err := opentracing.GlobalTracer().Inject(
		span.Context(),
		opentracing.TextMap,
		carrier,
	); err != nil {
		return nil, err
	}

	return MarshalTextMapCarrier(carrier)
}

func MarshalTextMapCarrier(carrier opentracing.TextMapCarrier) ([]byte, error) {
	data, err := json.Marshal(carrier)
	if err != nil {
		return nil, xerrors.Errorf("failed to marshal opentracing text map carrier: %w", err)
	}

	return data, nil
}

func UnmarshalTextMapCarrier(data []byte) (opentracing.TextMapCarrier, error) {
	var carrier opentracing.TextMapCarrier
	if err := json.Unmarshal(data, &carrier); err != nil {
		return nil, xerrors.Errorf("failed to unmarshal opentracing text map carrier: %w", err)
	}

	return carrier, nil
}
