package tracing

import (
	"github.com/opentracing/opentracing-go"
	olog "github.com/opentracing/opentracing-go/log"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling/tracetag"
	"a.yandex-team.ru/library/go/core/log"
)

func RootSpan(operation string, requestID string) opentracing.Span {
	t := defTracer.get()
	return t.StartSpan(operation, tracetag.RequestID(requestID))
}

func ChildSpan(operation string, parent opentracing.Span, tags ...opentracing.Tag) opentracing.Span {
	t := defTracer.get()
	var options []opentracing.StartSpanOption
	if parent != nil {
		options = append(options, opentracing.ChildOf(parent.Context()))
	}
	if len(tags) > 0 {
		for _, tag := range tags {
			options = append(options, tag)
		}
	}
	return t.StartSpan(operation, options...)
}

func FinishSpan(span opentracing.Span) {
	if span == nil {
		return
	}
	span.Finish()
}

func SpanEvent(span opentracing.Span, event string, fields ...log.Field) {
	if span == nil {
		return
	}
	spanFields := make([]olog.Field, len(fields)+1)
	spanFields[0] = olog.Event(event)
	for i, lf := range fields {
		spanFields[i+1] = convertLogField(lf)
	}

	span.LogFields(spanFields...)
}

func TagSpan(span opentracing.Span, tags ...opentracing.Tag) {
	if span == nil {
		return
	}
	for _, t := range tags {
		t.Set(span)
	}
}
