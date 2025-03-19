package tags

import (
	"context"

	"github.com/opentracing/opentracing-go"
)

type Setter func(span opentracing.Span)

func SetContext(ctx context.Context, setter Setter) opentracing.Span {
	span := opentracing.SpanFromContext(ctx)
	if span != nil {
		setter(span)
	}

	return span
}

func MultiSetContext(ctx context.Context, setters []Setter) opentracing.Span {
	span := opentracing.SpanFromContext(ctx)
	if span != nil {
		for _, setter := range setters {
			setter(span)
		}
	}

	return span
}
