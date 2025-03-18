package httpmetrics

import (
	"net/http"
	"net/http/httptest"
	"testing"

	"a.yandex-team.ru/library/go/core/metrics/solomon"
)

func fakeHandler(status int) http.Handler {
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		w.WriteHeader(status)
	})
}

func panicHandler(w http.ResponseWriter, r *http.Request) {
	panic("hello")
}

func TestMiddleware(t *testing.T) {
	r := solomon.NewRegistry(solomon.NewRegistryOpts())

	middleware := New(r, WithPathEndpoint(), WithHTTPCodes(404))

	w := httptest.NewRecorder()

	middleware(fakeHandler(200)).ServeHTTP(w, httptest.NewRequest("GET", "/items", nil))
	middleware(fakeHandler(404)).ServeHTTP(w, httptest.NewRequest("POST", "/users", nil))

	func() {
		defer func() { _ = recover() }()

		middleware(http.HandlerFunc(panicHandler)).ServeHTTP(w, httptest.NewRequest("POST", "/panic", nil))
	}()
}

func TestMiddleware_DefaultOptions(t *testing.T) {
	r := solomon.NewRegistry(solomon.NewRegistryOpts())

	middleware := New(r)

	w := httptest.NewRecorder()

	middleware(fakeHandler(200)).ServeHTTP(w, httptest.NewRequest("GET", "/items", nil))
}

func TestMiddleware_httpCodeDefaultTag(t *testing.T) {
	r := solomon.NewRegistry(solomon.NewRegistryOpts())

	middleware := New(r, WithHTTPCodes(400))

	w := httptest.NewRecorder()

	middleware(fakeHandler(405)).ServeHTTP(w, httptest.NewRequest("HEAD", "/items", nil))
}
