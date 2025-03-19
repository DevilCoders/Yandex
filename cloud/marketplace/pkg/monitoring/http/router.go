package monitoring

import (
	"net/http"
	"time"

	"github.com/go-chi/chi/v5"

	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/metrics/solomon"

	"a.yandex-team.ru/cloud/marketplace/pkg/ctxtools"
	"a.yandex-team.ru/cloud/marketplace/pkg/monitoring/status"
)

func initRouter(logger log.Logger, registry *solomon.Registry, statusChecker *status.StatusCollector) http.Handler {
	router := chi.NewRouter()

	router.Route("/monitorings/v1", func(r chi.Router) {
		r.With(accessLogCtx(logger.WithName("metrics"))).Get("/metrics/solomon", newMetricsHandler(registry))
		r.With(accessLogCtx(logger.WithName("ping"))).Get("/ping", newPingHandler(statusChecker))
		r.With(accessLogCtx(logger.WithName("status"))).Get("/status", newStatusHandler(statusChecker))
	})

	return router
}

type responseWriterWrapper struct {
	http.ResponseWriter

	statusCode int
}

func (w *responseWriterWrapper) WriteHeader(statusCode int) {
	w.statusCode = statusCode
	w.ResponseWriter.WriteHeader(statusCode)
}

func accessLogCtx(logger log.Logger) func(http.Handler) http.Handler {
	return func(next http.Handler) http.Handler {
		return http.HandlerFunc(func(rw http.ResponseWriter, r *http.Request) {

			wrapper := &responseWriterWrapper{
				ResponseWriter: rw,
			}

			start := time.Now()

			ctx := ctxtools.WithStructuredLogger(r.Context(), logger.Structured())
			next.ServeHTTP(wrapper, r.WithContext(ctx))

			elapsed := time.Since(start)

			httpStatus := wrapper.statusCode
			if httpStatus == 0 {
				httpStatus = 499
			}

			logger.Info("request completed",
				log.Duration("request_time", elapsed),
				log.String("user_agent", r.UserAgent()),
				log.String("remote_ip", r.RemoteAddr),
				log.String("path", r.URL.Path),
				log.Int("http_status", httpStatus),
			)
		})
	}
}
