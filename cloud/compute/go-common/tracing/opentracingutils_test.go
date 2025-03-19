package tracing

import (
	"context"
	"testing"
	"time"

	"github.com/opentracing/opentracing-go"
	"github.com/stretchr/testify/assert"

	"github.com/jonboulle/clockwork"
)

func init() {
	_, _ = InitJaegerTracing(Config{
		Address: "127.0.0.1:1234",
		Service: "test",
	})
}

func TestEnabledTracing(t *testing.T) {
	at := assert.New(t)
	ctx := context.Background()
	at.False(tracingEnabled(ctx))

	_, ctx = InitSpan(ctx, "asd")
	at.True(tracingEnabled(ctx))
}

func TestDisableTracingAfterNoop(t *testing.T) {
	at := assert.New(t)
	ctx := context.Background()
	at.False(tracingEnabled(ctx))

	_, ctx = InitSpan(ctx, "asd")
	at.True(tracingEnabled(ctx))

	_, ctx = StartSpanFromContext(ctx, "asd")
	at.True(tracingEnabled(ctx))

	noopSpan := noopTracer.StartSpan("asd")
	noopCtx := opentracing.ContextWithSpan(ctx, noopSpan)

	at.False(tracingEnabled(noopCtx))
	// start span from empty context
	_, rCtx := StartSpanFromContext(noopCtx, "qqq")
	at.False(tracingEnabled(rCtx))
}

func TestSpanRateLimiter_StartSpanFromContext(t *testing.T) {
	at := assert.New(t)
	ctx := context.Background()

	_, ctx = InitSpan(ctx, "asd")
	at.True(tracingEnabled(ctx))

	fc := clockwork.NewFakeClock()
	rl := SpanRateLimiter{
		InitialInterval: time.Second,
		MaxInterval:     time.Hour,
		clock:           fc,
	}
	_, rCtx := rl.StartSpanFromContext(ctx, "aaa")
	at.True(tracingEnabled(rCtx))

	fc.Advance(time.Millisecond)
	_, rCtx = rl.StartSpanFromContext(ctx, "aaa")
	at.False(tracingEnabled(rCtx))

	fc.Advance(time.Second)
	_, rCtx = rl.StartSpanFromContext(ctx, "aaa")
	at.True(tracingEnabled(rCtx))

	fc.Advance(time.Millisecond)
	_, rCtx = rl.StartSpanFromContext(ctx, "aaa")
	at.False(tracingEnabled(rCtx))

	fc.Advance(time.Second)
	_, rCtx = rl.StartSpanFromContext(ctx, "aaa")
	at.False(tracingEnabled(rCtx))

	fc.Advance(time.Second)
	_, rCtx = rl.StartSpanFromContext(ctx, "aaa")
	at.True(tracingEnabled(rCtx))
}
