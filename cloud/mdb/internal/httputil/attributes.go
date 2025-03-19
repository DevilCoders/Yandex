package httputil

import (
	"net/http"

	"a.yandex-team.ru/cloud/mdb/internal/request"
)

func RequestAttributesMiddleware(next http.Handler) http.Handler {
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		r = r.WithContext(request.WithAttributes(r.Context()))
		next.ServeHTTP(w, r)
	})
}
