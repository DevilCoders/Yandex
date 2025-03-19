package httputil

import (
	"net/http"

	"a.yandex-team.ru/cloud/mdb/internal/requestid"
	"a.yandex-team.ru/cloud/mdb/internal/tracing/tags"
)

var (
	headerRequestID = http.CanonicalHeaderKey("X-Request-Id")
)

// RequestIDMiddleware retrieves request id from header or generates a new one, then adds it to context
func RequestIDMiddleware(next http.Handler) http.Handler {
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		// Retrieve request id header. If its empty, generate new request id
		rid := r.Header.Get(headerRequestID)
		if rid == "" {
			rid = requestid.New()
		}

		// Add transport-agnostic request id to context
		r = r.WithContext(requestid.WithRequestID(r.Context(), rid))
		r = r.WithContext(requestid.WithLogField(r.Context(), rid))

		// Set request id to trace
		tags.RequestID.SetContext(r.Context(), rid)

		next.ServeHTTP(w, r)
	})
}
