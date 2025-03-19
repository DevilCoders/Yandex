package statuserver

import (
	"context"
	"net/http"
	"time"

	"github.com/go-chi/chi/v5"
	"github.com/go-chi/chi/v5/middleware"

	"a.yandex-team.ru/cloud/billing/go/public_api/internal/config"
	"a.yandex-team.ru/cloud/billing/go/public_api/internal/tooling/metrics"
	"a.yandex-team.ru/cloud/billing/go/public_api/internal/tooling/scope"
	"a.yandex-team.ru/library/go/core/log"
)

func createStatusHandler() http.Handler {
	router := chi.NewMux()
	router.Use(
		middleware.StripSlashes,
		middleware.GetHead,
		middleware.Recoverer,
	)

	router.Handle("/metrics", metrics.GetHandler())
	return router
}

func Create(cfg *config.Config) *http.Server {
	return &http.Server{
		Addr:         cfg.StatusListenAddr,
		ReadTimeout:  time.Millisecond * 100,
		WriteTimeout: time.Second * 5,
		IdleTimeout:  time.Second * 30,
		Handler:      createStatusHandler(),
	}
}

func Run(ctx context.Context, statusServer *http.Server) (err error) {
	ctx, cancel := context.WithCancel(ctx)
	defer cancel()

	go func() {
		defer cancel()

		scope.Logger(ctx).Info("start status server", log.String("address", statusServer.Addr))
		err = statusServer.ListenAndServe()
	}()

	if <-ctx.Done(); err != nil {
		scope.Logger(ctx).Error("status server failed with error", log.Error(err))
		return
	}

	scope.Logger(ctx).Error("shutdown status server")
	err = statusServer.Shutdown(ctx)
	if err != nil {
		scope.Logger(ctx).Error("shutdown status server failed with error", log.Error(err))
	}
	return
}
