package interceptors

import (
	"context"

	"google.golang.org/grpc"
)

// NewServerStreamWithContext returns server wrapped server stream that holds specified context
func NewServerStreamWithContext(stream grpc.ServerStream, ctx context.Context) grpc.ServerStream {
	return &serverStreamWithContext{ServerStream: stream, ctx: ctx}
}

type serverStreamWithContext struct {
	grpc.ServerStream
	ctx context.Context
}

func (s *serverStreamWithContext) Context() context.Context {
	return s.ctx
}
