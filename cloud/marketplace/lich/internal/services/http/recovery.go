package http

import (
	"net/http"

	"a.yandex-team.ru/cloud/marketplace/lich/pkg/panictools"
	"a.yandex-team.ru/cloud/marketplace/pkg/ctxtools"
)

func (s *Service) recoverMiddleware(next http.Handler) http.Handler {
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {

		defer func() {
			scoppedLogger := ctxtools.Logger(r.Context())

			if p := recover(); p != nil {
				panictools.LogRecovery(r.Context(), scoppedLogger.Logger(), p)
				s.sendAPIError(r.Context(), w, errInternal)
				return
			}

			scoppedLogger.Debug("request completed without panic")
		}()

		next.ServeHTTP(w, r)
	})
}
