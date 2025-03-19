package http

import (
	"net/http"

	"a.yandex-team.ru/library/go/core/log"

	"a.yandex-team.ru/cloud/marketplace/pkg/ctxtools"
)

func (s *Service) makeMethodNotAllowedHandler() http.HandlerFunc {
	return func(rw http.ResponseWriter, r *http.Request) {
		scoppedLogger := ctxtools.Logger(r.Context())
		scoppedLogger.Error("method not allowed", log.String("method", r.Method))

		rw.Header().Set("Content-type", "application/json")
		rw.WriteHeader(http.StatusUnauthorized)

		_, _ = rw.Write(
			[]byte(`{"code":"UnknownApiResource","internal":false,"message":"Unknown API resource.","status":3}`),
		)
	}
}
