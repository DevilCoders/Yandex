package interceptors

import (
	"context"
	"testing"

	"github.com/opentracing/opentracing-go"
	"github.com/opentracing/opentracing-go/mocktracer"
	"github.com/stretchr/testify/assert"
	"google.golang.org/grpc"
)

func Test_newUnaryClientInterceptorTracing(t *testing.T) {
	t.Run("client span is injected", func(t *testing.T) {
		tracer := mocktracer.New()
		interceptor := newUnaryClientInterceptorTracing(tracer)
		parent := tracer.StartSpan("parent_span")
		ctx := opentracing.ContextWithSpan(context.Background(), parent)
		_ = interceptor(ctx, "client_method", nil, nil, nil, func(ctx context.Context, method string, req, reply interface{}, cc *grpc.ClientConn, opts ...grpc.CallOption) error {
			child := opentracing.SpanFromContext(ctx)
			assert.NotEqual(t, parent, child)
			return nil
		})
	})
}
