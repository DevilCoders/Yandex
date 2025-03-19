package interceptors

import (
	"context"
	"strings"

	"github.com/gofrs/uuid"
	"google.golang.org/grpc"
	"google.golang.org/grpc/metadata"

	"a.yandex-team.ru/cloud/dataplatform/internal/logger"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/transfer_manager/go/pkg/contextutil"
	"a.yandex-team.ru/transfer_manager/go/pkg/util"
)

var (
	apiProtocolCtxKey = contextutil.NewContextKey()
)

const (
	requestIDHeaderKey = "x-request-id"
)

func extractReqID(meta metadata.MD, processedMetadataKeys map[string]struct{}) string {
	reqID, ok := meta[requestIDHeaderKey]
	processedMetadataKeys[requestIDHeaderKey] = struct{}{}
	if !ok || len(reqID) < 1 {
		return ""
	}
	return reqID[0]
}

func addRequestID(ctx context.Context, meta metadata.MD, info *grpc.UnaryServerInfo, processedMetadataKeys map[string]struct{}) context.Context {
	var clientProvidedReqID bool
	var reqID string
	var generateUUIDErr error
	if id := extractReqID(meta, processedMetadataKeys); id != "" {
		reqID = id
		clientProvidedReqID = true
	} else if uu, err := uuid.NewV1(); err == nil {
		reqID = uu.String()
		clientProvidedReqID = false
	} else {
		reqID = "unknown"
		generateUUIDErr = err
	}

	ctx = context.WithValue(ctx, requestIDKey, reqID)
	lgr := WithContextLogger(ctx)
	if generateUUIDErr != nil {
		lgr.Error("Cannot generate UUID for incoming request", log.String("method", info.FullMethod), log.Error(generateUUIDErr))
	} else if clientProvidedReqID {
		lgr.Info("Using request ID provided by the client", log.String("method", info.FullMethod))
	} else {
		lgr.Info("Using generated request ID", log.String("method", info.FullMethod))
	}
	return ctx
}

func addAPIProtocol(ctx context.Context, meta metadata.MD, processedMetadataKeys map[string]struct{}) context.Context {
	lgr := WithContextLogger(ctx)
	headerFields := meta.Get(APIProtocolHeaderName)
	processedMetadataKeys[APIProtocolHeaderName] = struct{}{}
	var protocol apiProtocol
	if len(headerFields) == 0 {
		protocol = APIProtocolGRPC
	} else {
		protocol = apiProtocol(headerFields[len(headerFields)-1])
	}
	lgr.Infof("Protocol type is %s", protocol)
	return context.WithValue(ctx, apiProtocolCtxKey, protocol)
}

func addIP(ctx context.Context, meta metadata.MD, processedMetadataKeys map[string]struct{}) context.Context {
	lgr := WithContextLogger(ctx)
	xRealIP, ok := meta[xRealIPHeaderName]
	if ok {
		if len(xRealIP) == 1 {
			lgr.Infof("x-real-ip is %s", xRealIP[0])
			ctx = context.WithValue(ctx, xRealIPKey, xRealIP[0])
		} else {
			lgr.Infof("x-real-ip length is not 1: %s", xRealIP)
		}
		processedMetadataKeys[xRealIPHeaderName] = struct{}{}
	}
	xForwardedFor, ok := meta[xForwardedForHeaderName]
	if ok {
		if len(xForwardedFor) == 1 {
			lgr.Infof("x-forwarded-for is %s", xForwardedFor[0])
			ctx = context.WithValue(ctx, xForwardedForKey, xForwardedFor[0])
		} else {
			lgr.Infof("x-forwarded-for length is not 1: %s", xForwardedFor)
		}
		processedMetadataKeys[xForwardedForHeaderName] = struct{}{}
	}
	return ctx
}

func addUserAgent(ctx context.Context, meta metadata.MD, processedMetadataKeys map[string]struct{}) context.Context {
	userAgent := meta[userAgentHeaderName]
	if len(userAgent) == 0 {
		return ctx
	}
	processedMetadataKeys[userAgentHeaderName] = struct{}{}
	return context.WithValue(ctx, userAgentKey, userAgent[0])
}

func addForwardedAgent(ctx context.Context, meta metadata.MD, processedMetadataKeys map[string]struct{}) context.Context {
	forwardedAgent := meta[xForwardedAgentHeaderName]
	if len(forwardedAgent) == 0 {
		return ctx
	}
	processedMetadataKeys[xForwardedAgentHeaderName] = struct{}{}
	return context.WithValue(ctx, xForwardedAgentKey, forwardedAgent[0])
}

func withRemainingMetadata(ctx context.Context, meta metadata.MD, processedMetadataKeys map[string]struct{}) context.Context {
	metaCopy := meta.Copy()

	// Delete processed and sensitive keys from the copied metadata
	for key := range metaCopy {
		if _, keyIsAlreadyProcessed := processedMetadataKeys[key]; keyIsAlreadyProcessed {
			delete(metaCopy, key)
		}
		lowercaseKey := strings.ToLower(key)
		if strings.Contains(lowercaseKey, "authorization") {
			delete(metaCopy, key)
		}
		if strings.Contains(lowercaseKey, "token") {
			delete(metaCopy, key)
		}
	}

	return context.WithValue(ctx, remainingMetadataCtxKey, metaCopy)
}

func WithRequestInfo(ctx context.Context, req interface{}, info *grpc.UnaryServerInfo, handler grpc.UnaryHandler) (interface{}, error) {
	meta, ok := metadata.FromIncomingContext(ctx)
	processedMetadataKeys := map[string]struct{}{}
	if ok {
		ctx = addRequestID(ctx, meta, info, processedMetadataKeys)
		ctx = addAPIProtocol(ctx, meta, processedMetadataKeys)
		ctx = addIP(ctx, meta, processedMetadataKeys)
		ctx = addUserAgent(ctx, meta, processedMetadataKeys)
		ctx = addForwardedAgent(ctx, meta, processedMetadataKeys)
	} else {
		logger.Log.Info("No metadata found")
	}
	ctx = context.WithValue(ctx, util.RequestMethodKey, info.FullMethod)
	ctx = withRemainingMetadata(ctx, meta, processedMetadataKeys)
	result, err := handler(ctx, req)
	if err != nil {
		WithContextLogger(ctx).Info("call error", log.Error(err))
	}
	return result, err
}
