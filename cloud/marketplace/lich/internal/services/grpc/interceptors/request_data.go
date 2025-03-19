package interceptors

import (
	"context"
	"encoding/hex"
	"fmt"
	"strconv"
	"strings"

	"github.com/gofrs/uuid"
	"google.golang.org/grpc"

	ctxtools "a.yandex-team.ru/cloud/marketplace/pkg/ctxtools"
	"a.yandex-team.ru/cloud/marketplace/pkg/grpc-tools/meta"
)

const (
	strOfZeros   = "0000000000000000"
	requestIDLen = 16
)

func NewRequestDataInjector() grpc.UnaryServerInterceptor {
	return func(
		ctx context.Context, request interface{}, info *grpc.UnaryServerInfo, handler grpc.UnaryHandler,
	) ( /* resp */ interface{}, error) {
		meta := meta.NewMeta(ctx)

		originalRequestID := meta.OriginalRequestID()

		userAgent := meta.UserAgent()
		remoteIP := meta.RemoteIP()

		ctx = wrapCtxWithRequestID(ctx, originalRequestID)
		ctx = ctxtools.WithRequestData(ctx, userAgent, remoteIP)

		return handler(ctx, request)
	}
}

func wrapCtxWithRequestID(ctx context.Context, requestID string) context.Context {
	requestID, uuidRequestID := getOrCreateRequestID(strings.ToLower(requestID))

	ctx = ctxtools.WithRequestID(ctx, requestID)
	ctx = ctxtools.WithLongRequestID(ctx, uuidRequestID)

	return ctxtools.WithOriginalRequestID(ctx, requestID)
}

func getOrCreateRequestID(rawRequestID string) (requestID string, longReqID string) {
	if rawRequestID == "" {
		return createRequestID()
	}

	if _, hexErr := strconv.ParseUint(rawRequestID, requestIDLen, 64); hexErr == nil {
		rawRequestID = fixRequestIDLen(rawRequestID)
		return rawRequestID, uuidStrFromHexInt(rawRequestID)
	}

	if uid := uuid.FromStringOrNil(rawRequestID); uid != uuid.Nil {
		requestID = hex.EncodeToString(uid.Bytes())
		longReqID = uuidStrFromHexInt(requestID)
		return
	}

	return
}

func fixRequestIDLen(v string) string {
	if len(v) >= requestIDLen {
		return v[:requestIDLen]
	}

	addZeros := requestIDLen - len(v)
	zeroedBase := strOfZeros[:addZeros]

	return fmt.Sprintf("%[1]s%[2]s", zeroedBase, v)
}

func uuidStrFromHexInt(v string) string {
	// "1e647b40441911ea" -> "1e647b40-4419-11ea-0000-000000000000"
	return fmt.Sprintf("%s-%s-%s-0000-000000000000", v[0:8], v[8:12], v[12:16])
}

func createRequestID() (requestID string, longReqID string) {
	uid := uuid.Must(uuid.NewV4())

	requestID = hex.EncodeToString(uid.Bytes())[:requestIDLen]
	longReqID = uuidStrFromHexInt(requestID)

	return
}
