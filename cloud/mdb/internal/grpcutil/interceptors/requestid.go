package interceptors

import (
	"context"

	"google.golang.org/grpc"
	"google.golang.org/grpc/metadata"

	"a.yandex-team.ru/cloud/mdb/internal/requestid"
	"a.yandex-team.ru/cloud/mdb/internal/tracing/tags"
)

const (
	headerRequestID = "x-request-id"
)

func withRequestID(ctx context.Context, rid string) context.Context {
	md, ok := metadata.FromOutgoingContext(ctx)
	if !ok {
		md = metadata.MD{}
	} else {
		md = md.Copy()
	}

	md.Set(headerRequestID, rid)
	return metadata.NewOutgoingContext(ctx, md)
}

func requestIDFromGRPC(ctx context.Context) (string, bool) {
	md, ok := metadata.FromIncomingContext(ctx)
	if !ok {
		return "", false
	}

	rid := md.Get(headerRequestID)
	if len(rid) != 1 {
		return "", false
	}

	return rid[0], true
}

// newUnaryServerInterceptorRequestID returns interceptor that retrieves request id from incoming gRPC request (or generates a new one) and adds it context
func newUnaryServerInterceptorRequestID() grpc.UnaryServerInterceptor {
	return func(ctx context.Context, req interface{}, info *grpc.UnaryServerInfo, handler grpc.UnaryHandler) (interface{}, error) {
		// Retrieve request id header. If its empty, generate new request id
		rid, ok := requestIDFromGRPC(ctx)
		if !ok {
			rid = requestid.New()
		}

		// Add transport-agnostic request id to context
		ctx = requestid.WithRequestID(ctx, rid)
		ctx = requestid.WithLogField(ctx, rid)

		// Set request id to trace
		tags.RequestID.SetContext(ctx, rid)

		return handler(ctx, req)
	}
}

// newStreamServerInterceptorRequestID returns interceptor that retrieves request id from incoming gRPC request (or generates a new one) and adds it context
func newStreamServerInterceptorRequestID() grpc.StreamServerInterceptor {
	return func(srv interface{}, ss grpc.ServerStream, info *grpc.StreamServerInfo, handler grpc.StreamHandler) error {
		ctx := ss.Context()
		// Retrieve request id header. If its empty, generate new request id
		rid, ok := requestIDFromGRPC(ctx)
		if !ok {
			rid = requestid.New()
		}

		// Add transport-agnostic request id to context
		ctx = requestid.WithRequestID(ctx, rid)
		ctx = requestid.WithLogField(ctx, rid)

		// Set request id to trace
		tags.RequestID.SetContext(ctx, rid)

		return handler(srv, NewServerStreamWithContext(ss, ctx))
	}
}

// newUnaryClientInterceptorRequestID returns interceptor that retrieves request id from context (or generates a new one) and adds it to outgoing gRPC request
func newUnaryClientInterceptorRequestID() grpc.UnaryClientInterceptor {
	return func(ctx context.Context, method string, req, reply interface{}, conn *grpc.ClientConn, invoker grpc.UnaryInvoker, opts ...grpc.CallOption) error {
		rid := requestid.New()
		ctx = requestid.WithRequestID(ctx, rid)
		ctx = requestid.WithLogField(ctx, rid)

		// Add request id to headers
		ctx = withRequestID(ctx, rid)

		// Set request id to trace
		tags.RequestID.SetContext(ctx, rid)

		return invoker(ctx, method, req, reply, conn, opts...)
	}
}

// newStreamClientInterceptorRequestID returns interceptor that retrieves request id from context (or generates a new one) and adds it to outgoing gRPC request
func newStreamClientInterceptorRequestID() grpc.StreamClientInterceptor {
	return func(ctx context.Context, desc *grpc.StreamDesc, cc *grpc.ClientConn, method string, streamer grpc.Streamer, opts ...grpc.CallOption) (grpc.ClientStream, error) {
		rid := requestid.New()
		ctx = requestid.WithRequestID(ctx, rid)
		ctx = requestid.WithLogField(ctx, rid)

		// Add request id to headers
		ctx = withRequestID(ctx, rid)

		// Set request id to trace
		tags.RequestID.SetContext(ctx, rid)

		return streamer(ctx, desc, cc, method, opts...)
	}
}
