package httpmiddleware

import (
	"context"
	"math/rand"
	"net/http"
	"strconv"

	"go.uber.org/zap"

	opentracing "github.com/opentracing/opentracing-go"
	"github.com/opentracing/opentracing-go/ext"

	"a.yandex-team.ru/cloud/compute/go-common/logging/ctxlog"
	"a.yandex-team.ru/cloud/compute/go-common/tracing"
)

func getTraceID(_ opentracing.SpanContext) uint64 {
	return uint64(rand.Int63())>>31 | uint64(rand.Int63())<<32
}

type statusCodeTracker struct {
	http.ResponseWriter
	status int
}

func (w *statusCodeTracker) WriteHeader(status int) {
	w.status = status
	w.ResponseWriter.WriteHeader(status)
}

// TracingHandler is a middleware which starts opentracing.Span and attaches logger
func TracingHandler(basectx context.Context, tr opentracing.Tracer, h http.Handler) http.Handler {
	hf := func(w http.ResponseWriter, r *http.Request) {
		// Extract SpanContext from HTTP-request
		otSpanCtx, err := tr.Extract(opentracing.HTTPHeaders, opentracing.HTTPHeadersCarrier(r.Header))
		if err != nil && err != opentracing.ErrSpanContextNotFound {
			ctxlog.G(basectx).Warn("failed to Extract SpanContext", zap.Error(err))
		}

		urlString := r.URL.String()
		// Start new server side span with extracted context
		serverSpan := tr.StartSpan(r.Method+" "+urlString, ext.RPCServerOption(otSpanCtx))
		defer serverSpan.Finish()

		ext.HTTPMethod.Set(serverSpan, r.Method)
		ext.HTTPUrl.Set(serverSpan, urlString)
		ext.Component.Set(serverSpan, "net/http")

		traceid := getTraceID(serverSpan.Context())
		tracelog := ctxlog.G(basectx).With(zap.Uint64("traceid", traceid))
		tracelog.Info("incoming request",
			zap.String("method", r.Method), zap.String("url", urlString), zap.String("remote", r.RemoteAddr),
		)
		ctx := tracing.WithTraceID(r.Context(), traceid)
		ctx = ctxlog.WithLogger(
			// Attach the span to a context
			opentracing.ContextWithSpan(ctx, serverSpan),
			// Attach logger to the span
			tracelog,
		)
		r = r.WithContext(ctx)

		w = &statusCodeTracker{w, http.StatusOK}
		w.Header().Set("Ot-Trace-id", strconv.FormatUint(traceid, 10))
		h.ServeHTTP(w, r)
		if sw, ok := w.(*statusCodeTracker); ok {
			ext.HTTPStatusCode.Set(serverSpan, uint16(sw.status))
		}
	}

	return http.HandlerFunc(hf)
}
