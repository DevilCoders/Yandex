package interceptors

import (
	"context"
	"runtime/debug"
	"time"

	"google.golang.org/grpc"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/metadata"
	"google.golang.org/grpc/status"

	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/transfer_manager/go/pkg/stats"
	grpcutil "a.yandex-team.ru/transfer_manager/go/pkg/util/grpc"
)

type (
	requestIDCtxField         struct{}
	xRealIPCtxField           struct{}
	xForwardedForCtxField     struct{}
	userAgentCtxField         struct{}
	xForwardedAgentCtxField   struct{}
	remainingMetadataCtxField struct{}
)

var (
	requestIDKey            = &requestIDCtxField{}
	xRealIPKey              = &xRealIPCtxField{}
	xForwardedForKey        = &xForwardedForCtxField{}
	userAgentKey            = &userAgentCtxField{}
	xForwardedAgentKey      = &xForwardedAgentCtxField{}
	remainingMetadataCtxKey = &remainingMetadataCtxField{}
)

type apiProtocol string

const (
	APIProtocolGRPC       = "grpc"
	APIProtocolHTTP       = "http"
	APIProtocolHeaderName = "Data-Transfer-Api-Protocol"
)

const (
	xRealIPHeaderName         = "x-real-ip"
	xForwardedForHeaderName   = "x-forwarded-for"
	userAgentHeaderName       = "user-agent"
	xForwardedAgentHeaderName = "x-forwarded-agent"
)

func getRequestID(ctx context.Context) string {
	lgr := WithContextLogger(ctx)
	reqIDTypeErased := ctx.Value(requestIDKey)
	if reqIDTypeErased == nil {
		lgr.Warn("No request ID found in context")
		return ""
	}
	reqID, ok := reqIDTypeErased.(string)
	if !ok {
		lgr.Warnf("Request ID is not a string but %T", reqIDTypeErased)
		return ""
	}
	return reqID
}

func WithReqIDHeader(ctx context.Context, req interface{}, info *grpc.UnaryServerInfo, handler grpc.UnaryHandler) (resp interface{}, err error) {
	lgr := WithContextLogger(ctx)
	response, err := handler(ctx, req)
	reqID := getRequestID(ctx)

	meta := metadata.Pairs(requestIDHeaderKey, reqID)
	if err := grpc.SendHeader(ctx, meta); err != nil {
		lgr.Warnf("Cannot send response header: %v", err)
	}
	return response, err
}

func WithPanicaHandler(ctx context.Context, req interface{}, info *grpc.UnaryServerInfo, handler grpc.UnaryHandler) (resp interface{}, err error) {
	lgr := WithContextLogger(ctx)
	defer func() {
		if r := recover(); r != nil {
			err = xerrors.Errorf("Panic: %v", r)
			lgr.Error("request panic", log.Error(err), log.Any("trace", string(debug.Stack())))
		}
	}()
	resp, err = handler(ctx, req)
	return
}

func WithRequestMetrics(serverMetrics *stats.ServerMethodStat) grpc.UnaryServerInterceptor {
	return func(ctx context.Context, req interface{}, info *grpc.UnaryServerInfo, handler grpc.UnaryHandler) (resp interface{}, err error) {
		start := time.Now()
		resp, err = handler(ctx, req)
		st, _ := status.FromError(err)
		serverMetrics.
			Method(info.FullMethod).
			Code(st.Code(), time.Since(start))
		return resp, err
	}
}

func WithUnwrapStatus(ctx context.Context, req interface{}, info *grpc.UnaryServerInfo, handler grpc.UnaryHandler) (resp interface{}, err error) {
	resp, err = handler(ctx, req)
	if ok, statusErr := grpcutil.UnwrapStatusError(err); ok {
		lgr := WithContextLogger(ctx)
		lgr.Info("Status error unwrapped", log.NamedError("original_error", err), log.NamedError("status_error", statusErr))
		return resp, statusErr
	}
	return resp, err
}

func WithDefaultStatus(ctx context.Context, req interface{}, info *grpc.UnaryServerInfo, handler grpc.UnaryHandler) (resp interface{}, err error) {
	resp, err = handler(ctx, req)
	if err != nil {
		if _, ok := status.FromError(err); !ok {
			return nil, status.Errorf(codes.InvalidArgument, "%v", err)
		}
	}
	return resp, err
}
