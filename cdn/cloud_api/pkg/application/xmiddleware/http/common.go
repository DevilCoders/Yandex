package http

import (
	"bytes"
	"fmt"
	"net/http"
	"runtime/debug"
	"time"

	"github.com/go-chi/chi/v5"
	"github.com/go-chi/chi/v5/middleware"

	"a.yandex-team.ru/cdn/cloud_api/pkg/application/xmiddleware"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
)

func RequestID(headerName string) func(http.Handler) http.Handler {
	return func(next http.Handler) http.Handler {
		return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
			ctx := r.Context()

			requestID := r.Header.Get(headerName)
			if requestID != "" {
				ctx = xmiddleware.SetRequestID(ctx, requestID)
			} else {
				ctx = xmiddleware.GenerateRequestID(ctx)
			}

			w.Header().Set(headerName, requestID)

			r = r.WithContext(ctx)
			next.ServeHTTP(w, r)
		})
	}
}

func Name(next http.Handler) http.Handler {
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		opName := "HTTP " + r.Method
		handler := ""

		rctx := chi.RouteContext(r.Context())

		var path string
		if rctx != nil && rctx.RoutePath != "" {
			path = rctx.RoutePath
		} else {
			path = r.URL.Path
		}

		if len(path) > 1 && path[len(path)-1] == '/' {
			path = path[:len(path)-1]
		}

		if ok := rctx.Routes.Match(rctx, r.Method, path); ok {
			handler = rctx.RoutePattern()
		}

		if handler != "" {
			opName += ": " + handler
		}

		ctx := xmiddleware.SetOperationName(r.Context(), handler)
		r = r.WithContext(ctx)
		next.ServeHTTP(w, r)
	})
}

func AccessLog(logger log.Logger) func(http.Handler) http.Handler {
	return func(next http.Handler) http.Handler {
		fn := func(w http.ResponseWriter, r *http.Request) {
			ww := middleware.NewWrapResponseWriter(w, r.ProtoMajor)

			startTime := time.Now()
			defer func() {
				ctxlog.Info(r.Context(), logger,
					"access log",
					log.String("remote_addr", r.RemoteAddr),
					log.String("method", r.Method),
					log.String("request_uri", r.RequestURI),
					log.Float64("request_time", time.Since(startTime).Seconds()),
					log.Int("status", ww.Status()),
					log.Int("bytes_sent", ww.BytesWritten()),
				)
			}()

			next.ServeHTTP(ww, r)
		}

		return http.HandlerFunc(fn)
	}
}

func Recover(logger log.Logger, responseStackTrace bool) func(http.Handler) http.Handler {
	return func(next http.Handler) http.Handler {
		return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
			defer func() {
				if p := recover(); p != nil {
					stackTrace := debug.Stack()
					msg := fmt.Sprintf("recovered from panic: %v", p)
					ctxlog.Error(r.Context(), logger, msg, log.String("stack_trace", string(stackTrace)), log.Bool("panic", true))

					w.WriteHeader(http.StatusInternalServerError)
					if responseStackTrace {
						buf := bytes.NewBuffer([]byte(msg + "\n"))
						_, _ = buf.Write(stackTrace)
						_, _ = w.Write(buf.Bytes())
					}
				}
			}()

			next.ServeHTTP(w, r)
		})
	}
}
