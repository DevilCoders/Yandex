package http

import (
	"net/http"

	"a.yandex-team.ru/cloud/marketplace/pkg/ctxtools"
)

func authCtx(next http.Handler) http.Handler {
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		ctx := ctxtools.WithAuthToken(
			r.Context(),
			extractYCSubjectToken(r.Header),
		)

		next.ServeHTTP(w, r.WithContext(ctx))
	})
}

func extractYCSubjectToken(h http.Header) string {
	return h.Get("X-YaCloud-SubjectToken")
}
