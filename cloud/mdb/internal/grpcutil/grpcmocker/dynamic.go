package grpcmocker

import (
	"net/http"

	"github.com/go-chi/chi/v5"

	"a.yandex-team.ru/cloud/mdb/internal/httputil"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type RegisterDynamicControlFunc func(r *chi.Mux) error

func serveDynamicControl(addr string, reg RegisterDynamicControlFunc, l log.Logger) error {
	logCfg := httputil.DefaultLoggingConfig()
	logCfg.LogResponseBody = true
	logCfg.LogRequestBody = true

	r := chi.NewRouter()
	r.Use(func(next http.Handler) http.Handler {
		return httputil.LoggingMiddleware(next, logCfg, l)
	})

	if err := reg(r); err != nil {
		return xerrors.Errorf("register dynamic handlers: %w", err)
	}

	go func() {
		l.Info("serving HTTP", log.String("addr", addr))
		if err := http.ListenAndServe(addr, r); err != nil {
			l.Errorf("listen and serve: %s", err)
		}
	}()

	return nil
}
