package http

import (
	"encoding/hex"
	"net/http"
	"strconv"
	"strings"

	"github.com/gofrs/uuid"

	"a.yandex-team.ru/library/go/core/log"

	"a.yandex-team.ru/cloud/marketplace/pkg/ctxtools"
	"a.yandex-team.ru/cloud/marketplace/pkg/logging"
)

const (
	requestIDLen = 16
	strOfZeros   = "0000000000000000"
)

func requestIDCtx(next http.Handler) http.Handler {
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		requestID := r.Header.Get("X-Request-ID")
		if requestID == "" {
			requestID = createRequestID()
		}

		requestID = normalizeRequestID(requestID)
		requestLogger := logging.Named("api",
			log.String("request_id", requestID),
		)

		ctx := ctxtools.WithStructuredLogger(r.Context(), requestLogger.Structured())
		ctx = ctxtools.WithRequestID(ctx, requestID)

		next.ServeHTTP(w, r.WithContext(ctx))
	})
}

func normalizeRequestID(rawRequestID string) string {
	if _, hexErr := strconv.ParseUint(rawRequestID, requestIDLen, 64); hexErr == nil {
		return fixRequestIDLen(rawRequestID)
	} else if uid := uuid.FromStringOrNil(rawRequestID); uid != uuid.Nil {
		return hex.EncodeToString(uid.Bytes())
	}

	return createRequestID()
}

func createRequestID() string {
	uid := uuid.Must(uuid.NewV4())
	return hex.EncodeToString(uid.Bytes())[:requestIDLen]
}

func fixRequestIDLen(v string) string {
	if len(v) >= requestIDLen {
		return v[:requestIDLen]
	}

	addZeros := requestIDLen - len(v)
	return strings.Join([]string{strOfZeros[:addZeros], v}, "")
}
