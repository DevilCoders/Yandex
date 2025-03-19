package tracing

import (
	"strings"

	"github.com/opentracing/opentracing-go"
	"google.golang.org/grpc/metadata"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling/logf"
)

func InjectGRPC(span opentracing.Span, md metadata.MD) {
	if span == nil {
		return
	}
	t := defTracer.get()
	mdWriter := metadataReaderWriter{MD: md}
	err := t.Inject(span.Context(), opentracing.HTTPHeaders, mdWriter)
	if err != nil {
		SpanEvent(span, "Tracer.Inject() failed", logf.Error(err))
	}
}

type metadataReaderWriter struct {
	metadata.MD
}

func (w metadataReaderWriter) Set(key, val string) {
	key = strings.ToLower(key)
	w.MD[key] = append(w.MD[key], val)
}

func (w metadataReaderWriter) ForeachKey(handler func(key, val string) error) error {
	for k, vals := range w.MD {
		for _, v := range vals {
			if err := handler(k, v); err != nil {
				return err
			}
		}
	}

	return nil
}
