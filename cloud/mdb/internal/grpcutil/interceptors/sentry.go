package interceptors

import (
	"context"
	"fmt"

	"google.golang.org/grpc"
	"google.golang.org/grpc/peer"
	"google.golang.org/protobuf/proto"

	"a.yandex-team.ru/cloud/mdb/internal/protoutil"
	"a.yandex-team.ru/cloud/mdb/internal/request"
	"a.yandex-team.ru/cloud/mdb/internal/sentry"
)

func sentryContextTags(ctx context.Context, req interface{}, method string) map[string]string {
	tags := make(map[string]string)

	if method != "" {
		tags["method"] = method
	}
	if p, ok := peer.FromContext(ctx); ok {
		tags["client_addr"] = p.Addr.String()
	}

	if req != nil {
		m, ok := req.(proto.Message)
		if ok {
			body, err := protoutil.MaskJSONMessage(m)
			if err != nil {
				tags["request.body"] = fmt.Sprintf("request body masking error: %s", err)
			} else {
				tags["request.body"] = body
			}
		}
	}

	reqTags := request.SentryTags(ctx)
	for k, v := range reqTags {
		tags[k] = v
	}

	return tags
}

func reportServerError(ctx context.Context, req interface{}, method string, err error) {
	if err == nil {
		return
	}

	if sentry.NeedReport(err) {
		sentry.GlobalClient().CaptureError(ctx, err, sentryContextTags(ctx, req, method))
	}
}

func newUnaryServerInterceptorSentry() grpc.UnaryServerInterceptor {
	return func(ctx context.Context, req interface{}, info *grpc.UnaryServerInfo, handler grpc.UnaryHandler) (interface{}, error) {
		resp, err := handler(ctx, req)
		reportServerError(ctx, req, info.FullMethod, err)
		return resp, err
	}
}

func newStreamServerInterceptorSentry() grpc.StreamServerInterceptor {
	return func(srv interface{}, ss grpc.ServerStream, info *grpc.StreamServerInfo, handler grpc.StreamHandler) error {
		err := handler(srv, ss)
		reportServerError(ss.Context(), nil, info.FullMethod, err)
		return err
	}
}
