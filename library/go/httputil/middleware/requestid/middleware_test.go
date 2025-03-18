package requestid

import (
	"context"
	"net/http"
	"net/http/httptest"
	"testing"

	"github.com/stretchr/testify/assert"
)

func TestRandomOnEmptyGeneratorHandler_wrap(t *testing.T) {
	// create a handler to use as "next" which will verify the request
	nextHandler := http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		val := FromContext(r.Context())
		assert.NotEmpty(t, val, "reqId was not set")
	})

	// create the handler to test, using our custom "next" handler
	handlerToTest := New(WithRequestIDGenerator(RandomOnEmptyGenerator))(nextHandler)

	// create a mock request to use
	req := httptest.NewRequest("GET", "http://testing", nil)

	// call the handler using a mock response recorder (we'll not use that anyway)
	handlerToTest.ServeHTTP(httptest.NewRecorder(), req)
}

func TestNopGeneratorHandler_wrap(t *testing.T) {
	// create a handler to use as "next" which will verify the request
	nextHandler := http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		val := FromContext(r.Context())
		assert.Equal(t, "arcadia", val, "reqId was set")
	})

	// create the handler to test, using our custom "next" handler
	handlerToTest := New()(nextHandler)

	// create a mock request to use
	req := httptest.NewRequest("GET", "http://testing", nil)
	req.Header.Set("X-Request-Id", "arcadia")

	// call the handler using a mock response recorder (we'll not use that anyway)
	handlerToTest.ServeHTTP(httptest.NewRecorder(), req)
}

func TestAppHostGeneratorHandler_wrap(t *testing.T) {
	// create a handler to use as "next" which will verify the request
	nextHandler := http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		val := FromContext(r.Context())
		assert.NotEmpty(t, val, "reqId was set")
	})

	// create the handler to test, using our custom "next" handler
	handlerToTest := New(WithRequestIDGenerator(NewAppHostGenerator()))(nextHandler)

	// create a mock request to use
	req := httptest.NewRequest("GET", "http://testing", nil)

	// call the handler using a mock response recorder (we'll not use that anyway)
	handlerToTest.ServeHTTP(httptest.NewRecorder(), req)
}

func TestFromContext(t *testing.T) {
	t.Run("not_exists", func(t *testing.T) {
		req := httptest.NewRequest("GET", "http://testing", nil)
		assert.Empty(t, FromContext(req.Context()))
	})

	t.Run("exists", func(t *testing.T) {
		req := httptest.NewRequest("GET", "http://testing", nil)
		ctx := context.WithValue(req.Context(), ctxKey{}, "arcadia")
		req = req.WithContext(ctx)
		assert.Equal(t, "arcadia", FromContext(req.Context()))
	})
}
