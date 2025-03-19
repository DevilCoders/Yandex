package api

import (
	"net/http"

	"a.yandex-team.ru/cloud/mdb/mdb-pillar-config/internal/auth"
)

func AuthFromRequest(next http.Handler) http.Handler {
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		accessID := r.Header["Access-Id"]
		accessSecret := r.Header["Access-Secret"]

		if len(accessID) == 1 && len(accessSecret) == 1 {
			r = r.WithContext(auth.WithAuth(r.Context(), accessID[0], accessSecret[0]))
		}

		next.ServeHTTP(w, r)
	})
}
