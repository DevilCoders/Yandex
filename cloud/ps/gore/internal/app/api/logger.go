package api

import (
	"net/http"
	"runtime/debug"
	"time"

	"a.yandex-team.ru/library/go/core/log"

	"github.com/go-chi/chi/v5/middleware"
)

func (srv *Server) mwLogger() func(next http.Handler) http.Handler {
	return func(next http.Handler) http.Handler {
		fn := func(w http.ResponseWriter, r *http.Request) {
			zlog := srv.BaseLogger.With(
				log.String("module", "http-log-wrapper"),
				log.String("requestID", r.Context().Value(middleware.RequestIDKey).(string)),
			)

			ww := middleware.NewWrapResponseWriter(w, r.ProtoMajor)

			t1 := time.Now()
			defer func() {
				t2 := time.Now()

				if rec := recover(); rec != nil {
					zlog.Error("error_request", log.ByteString("debug_stack", debug.Stack()))
					http.Error(ww, http.StatusText(http.StatusInternalServerError), http.StatusInternalServerError)
				}

				fields := []log.Field{
					log.String("remote_ip", r.RemoteAddr),
					log.String("host", r.Host),
					log.String("proto", r.Proto),
					log.String("method", r.Method),
					log.String("user_agent", r.Header.Get("User-Agent")),
					log.Int("status", ww.Status()),
					log.Float64("latency_ms", float64(t2.Sub(t1).Nanoseconds())/1000000.0),
					log.String("bytes_in", r.Header.Get("Content-Length")),
					log.Int("bytes_out", ww.BytesWritten()),
				}
				zlog.Info(r.RequestURI, fields...)
			}()

			next.ServeHTTP(ww, r)
		}
		return http.HandlerFunc(fn)
	}
}
