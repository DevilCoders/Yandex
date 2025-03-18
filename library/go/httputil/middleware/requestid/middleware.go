package requestid

import (
	"context"
	"net/http"
)

type ctxKey struct{}

type middleware struct {
	headers   []string
	generator GeneratorFunc
}

// New returns new instance of middleware that injects a request ID into the context of each request.
func New(opts ...MiddlewareOption) func(next http.Handler) http.Handler {
	m := middleware{
		headers:   []string{"X-Request-Id", "X-Req-Id", "X-Yandex-Req-Id"},
		generator: NopGenerator,
	}

	for _, opt := range opts {
		opt(&m)
	}

	return m.wrap
}

func (m *middleware) wrap(next http.Handler) http.Handler {
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		var requestID string
		var requestIDHeader string

		for _, requestIDHeader = range m.headers {
			requestID = r.Header.Get(requestIDHeader)
			if requestID != "" {
				break
			}
		}

		requestID = m.generator(requestID)

		w.Header().Set(requestIDHeader, requestID)

		ctx := context.WithValue(r.Context(), ctxKey{}, requestID)
		next.ServeHTTP(w, r.WithContext(ctx))
	})
}

// FromContext returns a request ID from the given context if one is present.
// Returns the empty string if a request ID cannot be found.
func FromContext(ctx context.Context) string {
	if ctx == nil {
		return ""
	}
	if val, ok := ctx.Value(ctxKey{}).(string); ok {
		return val
	}
	return ""
}
