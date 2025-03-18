package grpc

import (
	"context"
	"fmt"
	"runtime/debug"
	"time"

	"google.golang.org/genproto/googleapis/rpc/code"
	"google.golang.org/grpc"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/metadata"
	"google.golang.org/grpc/peer"
	"google.golang.org/grpc/status"

	"a.yandex-team.ru/cdn/cloud_api/pkg/application/xmiddleware"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
)

func requestIDFromMetadata(ctx context.Context, key string) string {
	md, ok := metadata.FromIncomingContext(ctx)
	if !ok {
		return ""
	}
	value := md.Get(key)
	if len(value) < 1 {
		return ""
	}

	return value[0]
}

func RequestID(metadataKey string) func(ctx context.Context, req interface{}, info *grpc.UnaryServerInfo, handler grpc.UnaryHandler) (interface{}, error) {
	return func(ctx context.Context, req interface{}, info *grpc.UnaryServerInfo, handler grpc.UnaryHandler) (interface{}, error) {
		requestID := requestIDFromMetadata(ctx, metadataKey)
		if requestID != "" {
			ctx = xmiddleware.SetRequestID(ctx, requestID)
		} else {
			ctx = xmiddleware.GenerateRequestID(ctx)
		}

		if err := grpc.SetHeader(ctx, metadata.Pairs(metadataKey, requestID)); err != nil {
			return nil, fmt.Errorf("set header: %w", err)
		}

		return handler(ctx, req)
	}
}

func Name(ctx context.Context, req interface{}, info *grpc.UnaryServerInfo, handler grpc.UnaryHandler) (interface{}, error) {
	ctx = xmiddleware.SetOperationName(ctx, info.FullMethod)
	return handler(ctx, req)
}

func AccessLog(logger log.Logger) func(ctx context.Context, req interface{}, info *grpc.UnaryServerInfo, handler grpc.UnaryHandler) (interface{}, error) {
	return func(ctx context.Context, req interface{}, info *grpc.UnaryServerInfo, handler grpc.UnaryHandler) (interface{}, error) {
		startTime := time.Now()
		resp, err := handler(ctx, req)
		if err != nil {
			ctxlog.Error(ctx, logger, "grpc response error",
				log.String("method", info.FullMethod),
				log.String("grpc_code", status.Code(err).String()),
				log.Float64("request_time", time.Since(startTime).Seconds()),
				log.String("remote_addr", remoteAddr(ctx)),
			)
		} else {
			ctxlog.Info(ctx, logger, "grpc response success",
				log.String("method", info.FullMethod),
				log.String("grpc_code", code.Code_OK.String()),
				log.Float64("request_time", time.Since(startTime).Seconds()),
				log.String("remote_addr", remoteAddr(ctx)),
			)
		}

		return resp, err
	}
}

func remoteAddr(ctx context.Context) string {
	if p, ok := peer.FromContext(ctx); ok {
		return p.Addr.String()
	} else {
		return ""
	}
}

func Recover(logger log.Logger) func(ctx context.Context, req interface{}, info *grpc.UnaryServerInfo, handler grpc.UnaryHandler) (_ interface{}, err error) {
	return func(ctx context.Context, req interface{}, info *grpc.UnaryServerInfo, handler grpc.UnaryHandler) (_ interface{}, err error) {
		defer func() {
			if p := recover(); p != nil {
				msg := fmt.Sprintf("recovered from panic: %v", p)
				err = status.Errorf(codes.Internal, msg)
				ctxlog.Errorf(ctx, logger, msg, log.String("stack_trace", string(debug.Stack())), log.Bool("panic", true))
			}
		}()

		return handler(ctx, req)
	}
}
