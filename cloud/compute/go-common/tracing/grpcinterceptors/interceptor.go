package grpcinterceptors

import (
	"math/rand"
	"strings"

	opentracing "github.com/opentracing/opentracing-go"
	"github.com/opentracing/opentracing-go/ext"
	openlog "github.com/opentracing/opentracing-go/log"

	"go.uber.org/zap"
	"go.uber.org/zap/zapcore"
	"golang.org/x/net/context"
	"google.golang.org/grpc"
	"google.golang.org/grpc/metadata"

	"a.yandex-team.ru/cloud/compute/go-common/logging/ctxlog"
	"a.yandex-team.ru/cloud/compute/go-common/pkg/metriclabels"
	"a.yandex-team.ru/cloud/compute/go-common/pkg/xrequestid"
	"a.yandex-team.ru/library/go/slices"
)

const (
	YcMetadataPrefix      = "yc-"
	YcRequestIDHeader     = "yc-request_id"
	YcOperationIDHeader   = "yc-operation_id"
	YcTaskIDHeader        = "yc-task_id"
	YcComputeFeatureFlags = "x-feature-flag"

	authorizedHeaderForOldPythonClient = "yc-authorization"

	// feature-flags
	SnapshotDetailsFlag = "snapshot-details"
)

func getTraceID(_ opentracing.SpanContext) uint64 {
	return uint64(rand.Int63())>>31 | uint64(rand.Int63())<<32
}

func getSpanID(_ opentracing.SpanContext) uint64 {
	return uint64(rand.Int63())>>31 | uint64(rand.Int63())<<32
}

func fillLoggingContextFromMetadata(ctx context.Context) context.Context {
	md, ok := metadata.FromIncomingContext(ctx)
	if !ok {
		return ctx
	}
	var contextValues []zapcore.Field
	for name, values := range md {
		if !strings.HasPrefix(name, YcMetadataPrefix) || name == authorizedHeaderForOldPythonClient {
			continue
		}
		contextValues = append(contextValues, zap.String(name[len(YcMetadataPrefix):], strings.Join(values, ",")))
	}

	ctx = ctxlog.WithLogger(ctx, ctxlog.G(ctx).With(contextValues...))

	if rqID, ok := md[YcRequestIDHeader]; ok {
		ctx = xrequestid.WithRequestID(ctx, strings.Join(rqID, ","))
	}

	ctx = metriclabels.WithMetricData(ctx, metriclabels.Labels{
		DetailLabels: slices.ContainsString(md[YcComputeFeatureFlags], SnapshotDetailsFlag),

		OperationID:   strings.Join(md[YcOperationIDHeader], ","),
		ComputeTaskID: strings.Join(md[YcTaskIDHeader], ","),
	})

	return ctx
}

// MetadataInterceptor adds custom metadata values to logging context and invokes a request.
func MetadataInterceptor() grpc.UnaryServerInterceptor {
	return func(ctx context.Context, req interface{}, info *grpc.UnaryServerInfo, handler grpc.UnaryHandler) (interface{}, error) {
		ctx = fillLoggingContextFromMetadata(ctx)
		return handler(ctx, req)
	}
}

// SpanMetadataInterceptor adds custom metadata values to logging context,
// extracts SpanContext, creates context-related logger and invokes a request.
func SpanMetadataInterceptor(tracer opentracing.Tracer) grpc.UnaryServerInterceptor {
	return func(ctx context.Context, req interface{}, info *grpc.UnaryServerInfo, handler grpc.UnaryHandler) (interface{}, error) {
		ctx = fillLoggingContextFromMetadata(ctx)

		var (
			spanContext opentracing.SpanContext
			err         error
		)

		md, ok := metadata.FromIncomingContext(ctx)
		if ok {
			spanContext, err = tracer.Extract(opentracing.HTTPHeaders, MetadataReaderWriter{md.Copy()})
			if err != nil && err != opentracing.ErrSpanContextNotFound {
				ctxlog.G(ctx).Warn("failed to Extract SpanContext", zap.Error(err))
			}
		}

		serverSpan := tracer.StartSpan(info.FullMethod, ext.RPCServerOption(spanContext))
		defer serverSpan.Finish()

		ctx = ctxlog.WithLogger(
			opentracing.ContextWithSpan(ctx, serverSpan),
			ctxlog.G(ctx).With(zap.Uint64("traceid", getTraceID(serverSpan.Context()))),
		)

		// Log span start to trace canceled requests
		ctxlog.G(ctx).Info("span started", zap.String("op", info.FullMethod),
			zap.Uint64("spanid", getSpanID(serverSpan.Context())))

		resp, rpcerr := handler(ctx, req)
		if err != nil {
			serverSpan.LogFields(openlog.Error(err))
		}
		return resp, rpcerr
	}
}

// UnaryInterceptor is a legacy alias for SpanMetadataInterceptor.
func UnaryInterceptor(tracer opentracing.Tracer) grpc.UnaryServerInterceptor {
	return SpanMetadataInterceptor(tracer)
}
