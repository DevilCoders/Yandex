package tooling

import (
	"context"

	"google.golang.org/grpc"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/metadata"
	"google.golang.org/grpc/status"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling/logf"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling/metrics"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling/tracetag"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/tooling/tracing"
)

const (
	requestIDHeaderName = "X-Request-Id"
)

func NewToolingUnaryInterceptor() grpc.UnaryClientInterceptor {
	return func(
		ctx context.Context,
		method string,
		req interface{},
		reply interface{},
		cc *grpc.ClientConn,
		invoker grpc.UnaryInvoker,
		opts ...grpc.CallOption,
	) error {
		storage := getStoreFromCtx(ctx)
		if storage == nil {
			return invoker(ctx, method, req, reply, cc, opts...)
		}
		md := getGRPCMeta(ctx)
		if storage.requestID != "" {
			md.Set(requestIDHeaderName, storage.requestID)
		}
		span := tracing.ChildSpan(method, storage.span, tracetag.RPCClientSpan(), tracetag.GRPCComponenet())
		defer span.Finish()
		tracing.InjectGRPC(span, md)

		ctx = metadata.NewOutgoingContext(ctx, md)
		clock := storage.getClock()
		start := clock.Now()
		metrics.GRPCStarted.WithLabelValues(metrics.GRPCStartLabels(method)...).Inc()

		err := invoker(ctx, method, req, reply, cc, opts...)
		code := getGRPCCode(err)

		doneLbls := metrics.GRPCDoneLabels(method, code)
		metrics.GRPCDuration.WithLabelValues(doneLbls...).Observe(float64(clock.Since(start).Microseconds()))

		if code != codes.OK {
			tracing.TagSpan(span, tracetag.Failed(), tracetag.GRPCError(code))
			tracing.SpanEvent(span, "grpc call failed", logf.Error(err))
		}

		if err == nil {
			storage.logger().Debug("grpc call done", logf.GRPCMethod(method), logf.GRPCCode(code))
			return nil
		}
		storage.logger().Error("grpc call failed", logf.GRPCMethod(method), logf.GRPCCode(code), logf.Error(err))
		return err
	}
}

func getGRPCMeta(ctx context.Context) metadata.MD {
	md, ok := metadata.FromOutgoingContext(ctx)
	if !ok {
		return metadata.New(nil)
	}
	return md.Copy()
}

func getGRPCCode(err error) codes.Code {
	if err == nil {
		return codes.OK
	}
	if s, ok := status.FromError(err); ok {
		return s.Code()
	}
	return codes.Unknown
}
