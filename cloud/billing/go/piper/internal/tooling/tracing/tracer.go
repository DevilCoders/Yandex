package tracing

import (
	"io"
	"sync"

	"github.com/opentracing/opentracing-go"
)

func SetTracer(tracer opentracing.Tracer, closer io.Closer) {
	defTracer.mu.Lock()
	defer defTracer.mu.Unlock()

	defTracer.jt = tracer
	defTracer.closer = closer
}

func CloseTracer() error {
	defTracer.mu.Lock()
	defer defTracer.mu.Unlock()

	defTracer.jt = nil
	if defTracer.closer != nil {
		return defTracer.closer.Close()
	}
	return nil
}

var defTracer tracer

type tracer struct {
	jt     opentracing.Tracer
	closer io.Closer

	mu sync.RWMutex
}

func (t *tracer) get() opentracing.Tracer {
	t.mu.RLock()
	defer t.mu.RUnlock()
	if t.jt != nil {
		return t.jt
	}
	return opentracing.NoopTracer{}
}
