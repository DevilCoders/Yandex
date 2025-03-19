package httputil

import (
	"fmt"
	"net/http"
	"net/http/httptest"
	"net/http/httputil"
	_ "net/http/pprof" // Populate http.DefaultServeMux
	"runtime/debug"

	"github.com/go-openapi/runtime/middleware"
	"github.com/opentracing-contrib/go-stdlib/nethttp"
	"github.com/opentracing/opentracing-go"

	"a.yandex-team.ru/cloud/mdb/internal/prometheus"
	"a.yandex-team.ru/cloud/mdb/internal/request"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
)

// ChainStandardMiddleware for use in any HTTP server
func ChainStandardMiddleware(next http.Handler, tracingServiceName string, cfg LoggingConfig, l log.Logger) http.Handler {
	return RequestAttributesMiddleware(
		RequestIDMiddleware(
			LoggingMiddleware(
				PanicMiddleware(
					prometheus.Middleware(
						TracingMiddleware(next, tracingServiceName),
						l,
					),
					l),
				cfg,
				l,
			),
		),
	)
}

// ChainStandardSwaggerMiddleware for use in any swagger-based HTTP server
func ChainStandardSwaggerMiddleware(next http.Handler, tracingServiceName string, mdwCtx *middleware.Context, cfg LoggingConfig, l log.Logger) http.Handler {
	return RequestAttributesMiddleware(
		RequestIDMiddleware(
			LoggingMiddleware(
				prometheus.Middleware(
					TracingSwaggerMiddleware(next, tracingServiceName, mdwCtx),
					l,
				),
				cfg,
				l,
			),
		),
	)
}

// TracingMiddleware creates tracing spans
func TracingMiddleware(next http.Handler, serviceName string) http.Handler {
	return tracingMiddleware(
		next, serviceName,
		func(r *http.Request) string {
			return fmt.Sprintf("HTTP %s %s", r.Method, r.URL)
		},
	)
}

// TracingSwaggerMiddleware creates tracing spans for swagger-based server
func TracingSwaggerMiddleware(next http.Handler, serviceName string, mdwCtx *middleware.Context) http.Handler {
	return tracingMiddleware(
		next,
		serviceName,
		func(r *http.Request) string {
			route, _, ok := mdwCtx.RouteInfo(r)
			if !ok {
				return fmt.Sprintf("HTTP %s %s", r.Method, r.URL)
			}

			return fmt.Sprintf("HTTP %s %s", r.Method, route.PathPattern)
		},
	)
}

func tracingMiddleware(next http.Handler, serviceName string, opNameFunc func(r *http.Request) string) http.Handler {
	return nethttp.Middleware(
		opentracing.GlobalTracer(),
		next,
		nethttp.MWComponentName(serviceName),
		nethttp.OperationNameFunc(opNameFunc),
	)
}

// LoggingMiddleware logs http request, also logs request and response body if required
func LoggingMiddleware(next http.Handler, cfg LoggingConfig, l log.Logger) http.Handler {
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		lfds := []log.Field{
			log.String("ingress", "HTTP"),
			log.String("ingress_http_method", r.Method),
			log.String("ingress_url", r.URL.String()),
		}
		r = r.WithContext(ctxlog.WithFields(r.Context(), lfds...))

		ctxlog.Debug(r.Context(), l, "server request started")
		defer ctxlog.Debug(r.Context(), l, "server request finished")

		if cfg.LogRequestBody {
			dump, err := httputil.DumpRequest(r, true)
			if err != nil {
				ctxlog.Error(r.Context(), l, "failed to dump server request body", log.Error(err))
			} else {
				ctxlog.Debugf(r.Context(), l, "server request body:\n%s", dump)
			}
		}

		var rec *httptest.ResponseRecorder
		if cfg.LogResponseBody {
			rec = httptest.NewRecorder()
			next.ServeHTTP(rec, r)
		} else {
			next.ServeHTTP(w, r)
		}

		l = log.With(l, request.LogFields(r.Context())...)

		if rec != nil {
			dump, err := httputil.DumpResponse(rec.Result(), true)
			if err != nil {
				ctxlog.Error(r.Context(), l, "failed to dump server response body", log.Error(err))
			} else {
				ctxlog.Debugf(r.Context(), l, "server response body:\n%s", dump)
			}

			sendRecordedResponse(w, rec)
		}
	})
}

func sendRecordedResponse(w http.ResponseWriter, rec *httptest.ResponseRecorder) {
	// Now write recorded response to the response writer
	// Copy headers
	for k, v := range rec.Header() {
		w.Header()[k] = v
	}

	// Write copied headers and code
	w.WriteHeader(rec.Code)

	// Write body
	data := rec.Body.Bytes()
	_, _ = w.Write(data)
}

func PanicMiddleware(next http.Handler, l log.Logger) http.Handler {
	return http.HandlerFunc(func(w http.ResponseWriter, req *http.Request) {
		defer func() {
			if p := recover(); p != nil {
				l.Error(fmt.Sprintf("panicked: %+v\n%s", p, string(debug.Stack())))

				var err error
				switch pval := p.(type) {
				case error:
					err = pval
				default:
					err = fmt.Errorf("panicked with: %+v", pval)
				}

				ReportErrorToSentry(err, req)
				panic(p)
			}
		}()
		next.ServeHTTP(w, req)
	})
}
