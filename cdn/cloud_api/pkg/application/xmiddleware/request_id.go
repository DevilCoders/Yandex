package xmiddleware

import (
	"context"

	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/strm/common/go/pkg/core/xlog"
	"a.yandex-team.ru/strm/common/go/pkg/xutil"
)

type requestIDCtxKey uint8

const (
	RequestIDHeaderKey = "X-Request-ID"

	requestIDKey requestIDCtxKey = iota

	requestIDLen = 8
)

func GenerateRequestID(ctx context.Context) context.Context {
	requestID := xutil.RandomHexString(requestIDLen)
	return SetRequestID(ctx, requestID)
}

func SetRequestID(ctx context.Context, requestID string) context.Context {
	ctx = ctxlog.WithFields(ctx, xlog.RequestIDField(requestID))
	return context.WithValue(ctx, requestIDKey, requestID)
}

func GetRequestID(ctx context.Context) string {
	value := ctx.Value(requestIDKey)
	if requestID, ok := value.(string); ok {
		return requestID
	}

	return ""
}
