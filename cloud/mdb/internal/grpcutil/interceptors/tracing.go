package interceptors

import (
	"context"
	"io"
	"runtime"
	"strings"
	"sync/atomic"

	"github.com/opentracing/opentracing-go"
	"github.com/opentracing/opentracing-go/ext"
	otlog "github.com/opentracing/opentracing-go/log"
	"google.golang.org/grpc"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/metadata"
	"google.golang.org/grpc/status"

	"a.yandex-team.ru/cloud/mdb/internal/tracing"
	"a.yandex-team.ru/cloud/mdb/internal/tracing/tags"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func extractSpanContext(ctx context.Context, tracer opentracing.Tracer) (opentracing.SpanContext, error) {
	md, ok := metadata.FromIncomingContext(ctx)
	if !ok {
		md = metadata.New(nil)
	}

	return tracer.Extract(opentracing.HTTPHeaders, metadataReaderWriter{MD: md})
}

func newUnaryServerInterceptorTracing(tracer opentracing.Tracer, l log.Logger) grpc.UnaryServerInterceptor {
	return func(ctx context.Context, req interface{}, info *grpc.UnaryServerInfo, handler grpc.UnaryHandler) (interface{}, error) {
		spanContext, err := extractSpanContext(ctx, tracer)
		if err != nil && !xerrors.Is(err, opentracing.ErrSpanContextNotFound) {
			ctxlog.Error(ctx, l, "failed extract span context", log.Error(err))
		}

		span := tracer.StartSpan(info.FullMethod, ext.RPCServerOption(spanContext), componentTag)
		defer span.Finish()
		ctx = opentracing.ContextWithSpan(ctx, span)

		resp, err := handler(ctx, req)
		if err != nil {
			setSpanTags(span, err)
			tracing.SetErrorOnSpan(span, err)
		}

		return resp, err
	}
}

func newStreamServerInterceptorTracing(tracer opentracing.Tracer, l log.Logger) grpc.StreamServerInterceptor {
	return func(srv interface{}, ss grpc.ServerStream, info *grpc.StreamServerInfo, handler grpc.StreamHandler) error {
		spanContext, err := extractSpanContext(ss.Context(), tracer)
		if err != nil && !xerrors.Is(err, opentracing.ErrSpanContextNotFound) {
			ctxlog.Error(ss.Context(), l, "failed extract span context", log.Error(err))
		}

		span := tracer.StartSpan(info.FullMethod, ext.RPCServerOption(spanContext), componentTag)
		defer span.Finish()
		ss = &openTracingServerStream{
			ServerStream: ss,
			ctx:          opentracing.ContextWithSpan(ss.Context(), span),
		}

		if err = handler(srv, ss); err != nil {
			setSpanTags(span, err)
			tracing.SetErrorOnSpan(span, err)
		}

		return err
	}
}

type openTracingServerStream struct {
	grpc.ServerStream
	ctx context.Context
}

func (ss *openTracingServerStream) Context() context.Context {
	return ss.ctx
}

func setSpanTags(span opentracing.Span, err error) {
	code := codes.Unknown
	if s, ok := status.FromError(err); ok {
		code = s.Code()
	}

	responseCodeTag.Set(span, int(code))
}

var (
	componentTag    = tags.StringTagName(ext.Component).Tag("gRPC")
	responseCodeTag = tags.IntTagName("grpc.response.code")
)

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

func newUnaryClientInterceptorTracing(tracer opentracing.Tracer) grpc.UnaryClientInterceptor {
	return func(ctx context.Context, method string, req, resp interface{}, cc *grpc.ClientConn, invoker grpc.UnaryInvoker, opts ...grpc.CallOption) error {
		var parentCtx opentracing.SpanContext
		if parent := opentracing.SpanFromContext(ctx); parent != nil {
			parentCtx = parent.Context()
		}

		span := tracer.StartSpan(
			method,
			opentracing.ChildOf(parentCtx),
			ext.SpanKindRPCClient,
			componentTag,
		)
		defer span.Finish()
		ctx = injectSpanContext(ctx, tracer, span)

		if err := invoker(ctx, method, req, resp, cc, opts...); err != nil {
			setSpanTags(span, err)
			tracing.SetErrorOnSpan(span, err)
			return err
		}

		return nil
	}
}

func newStreamClientInterceptorTracing(tracer opentracing.Tracer) grpc.StreamClientInterceptor {
	return func(ctx context.Context, desc *grpc.StreamDesc, cc *grpc.ClientConn, method string, streamer grpc.Streamer, opts ...grpc.CallOption) (grpc.ClientStream, error) {
		var parentCtx opentracing.SpanContext
		if parent := opentracing.SpanFromContext(ctx); parent != nil {
			parentCtx = parent.Context()
		}

		span := tracer.StartSpan(
			method,
			opentracing.ChildOf(parentCtx),
			ext.SpanKindRPCClient,
			componentTag,
		)
		ctx = injectSpanContext(ctx, tracer, span)
		cs, err := streamer(ctx, desc, cc, method, opts...)
		if err != nil {
			setSpanTags(span, err)
			tracing.SetErrorOnSpan(span, err)
			span.Finish()
			return cs, err
		}

		return newOpenTracingClientStream(cs, method, desc, span), nil
	}
}

func newOpenTracingClientStream(cs grpc.ClientStream, method string, desc *grpc.StreamDesc, span opentracing.Span) grpc.ClientStream {
	finishChan := make(chan struct{})

	isFinished := new(int32)
	*isFinished = 0
	finishFunc := func(err error) {
		if !atomic.CompareAndSwapInt32(isFinished, 0, 1) {
			return
		}
		close(finishChan)
		defer span.Finish()
		if err != nil {
			setSpanTags(span, err)
			tracing.SetErrorOnSpan(span, err)
		}
	}
	go func() {
		select {
		case <-finishChan:
		case <-cs.Context().Done():
			finishFunc(cs.Context().Err())
		}
	}()
	otcs := &openTracingClientStream{
		ClientStream: cs,
		desc:         desc,
		finishFunc:   finishFunc,
	}

	runtime.SetFinalizer(otcs, func(otcs *openTracingClientStream) {
		otcs.finishFunc(nil)
	})
	return otcs
}

type openTracingClientStream struct {
	grpc.ClientStream
	desc       *grpc.StreamDesc
	finishFunc func(error)
}

func (cs *openTracingClientStream) Header() (metadata.MD, error) {
	md, err := cs.ClientStream.Header()
	if err != nil {
		cs.finishFunc(err)
	}
	return md, err
}

func (cs *openTracingClientStream) SendMsg(m interface{}) error {
	err := cs.ClientStream.SendMsg(m)
	if err != nil {
		cs.finishFunc(err)
	}
	return err
}

func (cs *openTracingClientStream) RecvMsg(m interface{}) error {
	err := cs.ClientStream.RecvMsg(m)
	if err == io.EOF {
		cs.finishFunc(nil)
		return err
	} else if err != nil {
		cs.finishFunc(err)
		return err
	}
	if !cs.desc.ServerStreams {
		cs.finishFunc(nil)
	}
	return err
}

func (cs *openTracingClientStream) CloseSend() error {
	err := cs.ClientStream.CloseSend()
	if err != nil {
		cs.finishFunc(err)
	}
	return err
}

func injectSpanContext(ctx context.Context, tracer opentracing.Tracer, clientSpan opentracing.Span) context.Context {
	md, ok := metadata.FromOutgoingContext(ctx)
	if !ok {
		md = metadata.New(nil)
	} else {
		md = md.Copy()
	}

	mdWriter := metadataReaderWriter{MD: md}
	if err := tracer.Inject(clientSpan.Context(), opentracing.HTTPHeaders, mdWriter); err != nil {
		clientSpan.LogFields(otlog.String("event", "Tracer.Inject() failed"), otlog.Error(err))
	}

	ctx = opentracing.ContextWithSpan(ctx, clientSpan)
	return metadata.NewOutgoingContext(ctx, md)
}
