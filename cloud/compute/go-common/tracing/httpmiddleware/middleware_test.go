package httpmiddleware

import (
	"context"
	"net/http"
	"net/http/httptest"
	"testing"

	opentracing "github.com/opentracing/opentracing-go"

	"github.com/stretchr/testify/assert"
)

func TestOpenTracingHTTPMiddleware(t *testing.T) {
	assrt := assert.New(t)

	handler := http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		w.WriteHeader(http.StatusForbidden)
		w.Header().Set("X-Header", "value")
		assrt.NotEqual(context.Background(), r.Context())
	})

	r := httptest.NewRequest("GET", "http://example.com/foo", nil)
	w := httptest.NewRecorder()
	TracingHandler(context.Background(), opentracing.GlobalTracer(), handler).ServeHTTP(w, r)

	assrt.Equal(w.Code, http.StatusForbidden)
	assrt.NotEmpty(w.Header().Get("Ot-Trace-Id"))
	assrt.Equal(w.Header().Get("X-Header"), "value")
}
