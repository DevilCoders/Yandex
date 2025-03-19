package interceptors

import (
	"context"
	"fmt"
	"time"

	"google.golang.org/grpc"
	"google.golang.org/grpc/peer"
	"google.golang.org/grpc/status"
	"google.golang.org/protobuf/proto"

	"a.yandex-team.ru/cloud/mdb/internal/protoutil"
	"a.yandex-team.ru/cloud/mdb/internal/request"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
)

type LoggingConfig struct {
	// LogRequestBody defines if request body should be logged.
	// ATTENTION: It logs EVERYTHING within the request body, does not log metadata. NOT FOR PRODUCTION USE.
	LogRequestBody bool `yaml:"log_request_body"`
	// LogResponseBody defines if response body should be logged
	// ATTENTION: It logs EVERYTHING within the response body, does not log metadata. NOT FOR PRODUCTION USE.
	LogResponseBody bool `yaml:"log_response_body"`
}

func DefaultLoggingConfig() LoggingConfig {
	return LoggingConfig{}
}

// newUnaryServerInterceptorLogging returns interceptor that logs request's metadata and times handler's invocation
func newUnaryServerInterceptorLogging(cfg LoggingConfig, l log.Logger) grpc.UnaryServerInterceptor {
	return func(ctx context.Context, req interface{}, info *grpc.UnaryServerInfo, handler grpc.UnaryHandler) (interface{}, error) {
		ctx = ctxlog.WithFields(ctx, log.String("ingress", "gRPC"), log.String("ingress_grpc_method", info.FullMethod))
		p, ok := peer.FromContext(ctx)
		if ok {
			ctx = ctxlog.WithFields(ctx, log.String("client_addr", p.Addr.String()))
		}

		msg := "server unary request started"
		if cfg.LogRequestBody {
			msg = formatRequestBodyLog(msg, req)
		}
		ctxlog.Debug(ctx, l, msg)

		ts := time.Now()
		resp, err := handler(ctx, req)
		ctx = ctxlog.WithFields(ctx, append(request.LogFields(ctx), log.Duration("duration", time.Since(ts)))...)
		if err != nil {
			st := status.Convert(err)
			ctx = ctxlog.WithFields(ctx, log.Error(err), log.Any("error_details", st.Details()))
		}

		msg = "server unary request finished"
		if cfg.LogResponseBody {
			msg = formatResponseBodyLog(msg, resp)
		}
		ctxlog.Debug(ctx, l, msg)
		return resp, err
	}
}

// newStreamServerInterceptorLogging returns interceptor that logs request's metadata and times handler's invocation
func newStreamServerInterceptorLogging(l log.Logger) grpc.StreamServerInterceptor {
	return func(srv interface{}, ss grpc.ServerStream, info *grpc.StreamServerInfo, handler grpc.StreamHandler) error {
		ctx := ss.Context()
		ctx = ctxlog.WithFields(ctx, log.String("ingress", "gRPC"), log.String("ingress_grpc_method", info.FullMethod))
		p, ok := peer.FromContext(ctx)
		if ok {
			ctx = ctxlog.WithFields(ctx, log.String("client_addr", p.Addr.String()))
		}
		ctxlog.Debug(ctx, l, "server stream request started")

		ts := time.Now()
		err := handler(srv, NewServerStreamWithContext(ss, ctx))
		ctx = ctxlog.WithFields(ctx, log.Duration("duration", time.Since(ts)))
		if err != nil {
			st := status.Convert(err)
			ctx = ctxlog.WithFields(ctx, log.Error(err), log.Any("error_details", st.Details()))
		}
		ctxlog.Debug(ctx, l, "server stream request finished")
		return err
	}
}

// newUnaryClientInterceptorLogging returns interceptor that logs request's metadata and times call's invocation
func newUnaryClientInterceptorLogging(cfg LoggingConfig, l log.Logger) grpc.UnaryClientInterceptor {
	return func(ctx context.Context, method string, req, reply interface{}, conn *grpc.ClientConn, invoker grpc.UnaryInvoker, opts ...grpc.CallOption) error {
		lfds := []log.Field{
			log.String("egress", "gRPC"),
			log.String("egress_grpc_method", method),
			log.String("egress_target", conn.Target()),
		}
		ctx = ctxlog.WithFields(ctx, lfds...)

		msg := "client unary request started"
		if cfg.LogRequestBody {
			msg = formatRequestBodyLog(msg, req)
		}
		ctxlog.Debug(ctx, l, msg)

		ts := time.Now()
		err := invoker(ctx, method, req, reply, conn, opts...)

		lfds = lfds[:0]
		lfds = append(lfds, log.Duration("duration", time.Since(ts)))
		if err != nil {
			st := status.Convert(err)
			lfds = append(lfds, log.Error(err), log.Any("error_details", st.Details()))
		}

		msg = "client unary request finished"
		if cfg.LogResponseBody {
			msg = formatResponseBodyLog(msg, reply)
		}
		ctxlog.Debug(ctx, l, msg, lfds...)
		return err
	}
}

// newStreamClientInterceptorLogging returns interceptor that logs request's metadata and times call's invocation
func newStreamClientInterceptorLogging(l log.Logger) grpc.StreamClientInterceptor {
	return func(ctx context.Context, desc *grpc.StreamDesc, cc *grpc.ClientConn, method string, streamer grpc.Streamer, opts ...grpc.CallOption) (grpc.ClientStream, error) {
		ctx = ctxlog.WithFields(ctx, log.String("egress", "gRPC"), log.String("egress_grpc_method", method))
		p, ok := peer.FromContext(ctx)
		if ok {
			ctx = ctxlog.WithFields(ctx, log.String("client_addr", p.Addr.String()))
		}
		ctxlog.Debug(ctx, l, "client stream request started")

		ts := time.Now()
		cs, err := streamer(ctx, desc, cc, method, opts...)
		ctx = ctxlog.WithFields(ctx, log.Duration("duration", time.Since(ts)))
		if err != nil {
			st := status.Convert(err)
			ctx = ctxlog.WithFields(ctx, log.Error(err), log.Any("error_details", st.Details()))
		}
		ctxlog.Debug(ctx, l, "client stream request finished")
		return cs, err
	}
}
func formatRequestBodyLog(msg string, req interface{}) string {
	return formatBodyLog(msg, req, "request")
}

func formatResponseBodyLog(msg string, req interface{}) string {
	return formatBodyLog(msg, req, "response")
}

func formatBodyLog(msg string, req interface{}, reqresp string) string {
	m, ok := req.(proto.Message)
	if !ok {
		return fmt.Sprintf("%s (%s body is NOT a proto.Message)", msg, reqresp)
	}

	body, err := protoutil.MaskJSONMessage(m)
	if err != nil {
		return fmt.Sprintf("%s (%s body masking error: %s)", msg, reqresp, err)
	}

	return fmt.Sprintf("%s with %s body:\n%s\n", msg, reqresp, body)
}
