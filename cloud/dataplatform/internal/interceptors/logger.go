package interceptors

import (
	"context"
	"fmt"
	"strings"
	"time"

	"google.golang.org/grpc"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/metadata"
	"google.golang.org/grpc/status"

	"a.yandex-team.ru/cloud/dataplatform/internal/logger"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/transfer_manager/go/pkg/auth"
	"a.yandex-team.ru/transfer_manager/go/pkg/util"
)

func WithContextLogger(ctx context.Context) log.Logger {
	fields := make([]log.Field, 0, 1)
	if v := ctx.Value(requestIDKey); v != nil {
		if sv, ok := v.(string); ok {
			fields = append(fields, log.String("request-id", sv))
		}
	}
	if subject, ok := auth.SubjectFromContext(ctx); ok {
		if stringer, ok := subject.(fmt.Stringer); ok {
			fields = append(fields, log.String("user", stringer.String()))
		}
	}
	if protocol := ctx.Value(apiProtocolCtxKey); protocol != nil {
		if downcastedProtocol, ok := protocol.(apiProtocol); ok {
			fields = append(fields, log.String("api-protocol", string(downcastedProtocol)))
		}
	}
	if xRealIP := ctx.Value(xRealIPKey); xRealIP != nil {
		if stringxRealIP, ok := xRealIP.(string); ok {
			fields = append(fields, log.String(xRealIPHeaderName, stringxRealIP))
		}
	}
	if xForwardedFor := ctx.Value(xForwardedForKey); xForwardedFor != nil {
		if stringXForwardedForKey, ok := xForwardedFor.(string); ok {
			fields = append(fields, log.String(xForwardedForHeaderName, stringXForwardedForKey))
		}
	}
	if userAgent := ctx.Value(userAgentKey); userAgent != nil {
		if stringUserAgent, ok := userAgent.(string); ok {
			fields = append(fields, log.String("user-agent", stringUserAgent))
		}
	}
	if forwardedAgent := ctx.Value(xForwardedAgentKey); forwardedAgent != nil {
		if stringForwardedAgent, ok := forwardedAgent.(string); ok {
			fields = append(fields, log.String("forwarded-agent", stringForwardedAgent))
		}
	}
	if remainingMetadataTypeErased := ctx.Value(remainingMetadataCtxKey); remainingMetadataTypeErased != nil {
		if remainingMetadata, ok := remainingMetadataTypeErased.(metadata.MD); ok {
			fields = append(fields, log.Any("remaining-metadata", remainingMetadata))
		}
	}

	// TODO: get rid of global logger.Log
	return log.With(logger.Log, fields...)
}

func WithCallLogger(ctx context.Context, req interface{}, info *grpc.UnaryServerInfo, handler grpc.UnaryHandler) (resp interface{}, err error) {
	lgr := WithContextLogger(ctx)

	if !strings.Contains(info.FullMethod, "ConsoleService") {
		callStartedFieldNames := util.FieldNames{MainName: "request", StringFieldName: "request_string"}
		callStartedLogFields := util.LogFields(ctx, req, callStartedFieldNames, lgr)
		lgr.Info("Call started", callStartedLogFields...)
	}
	startTime := time.Now()
	resp, err = handler(ctx, req)

	callFinishedFieldNames := util.FieldNames{MainName: "response", StringFieldName: "response_string"}
	callFinishedLogFields := util.LogFields(ctx, resp, callFinishedFieldNames, lgr)
	callFinishedLogFields = append(callFinishedLogFields, log.Duration("latency", time.Since(startTime)))
	callFinishedLogFields = append(callFinishedLogFields, log.Error(err))
	lgr.Info("Call finished", callFinishedLogFields...)

	if err != nil {
		if _, ok := status.FromError(err); !ok {
			return nil, status.Errorf(codes.InvalidArgument, "%v", err)
		}
	}
	return resp, err
}
