package recovery

import (
	"net/http"

	"a.yandex-team.ru/library/go/core/log"
)

type MiddlewareOpt func(*middleware)

// WithLogger sets custom logger to middleware.
// If none given - nop.Logger used by default.
func WithLogger(l log.Structured) MiddlewareOpt {
	return func(mw *middleware) {
		mw.l = l
	}
}

// WithStatusCode sets status code to failed request if possible
// Error contains original panic cause
func WithCallBack(callback func(http.ResponseWriter, *http.Request, error)) MiddlewareOpt {
	return func(mw *middleware) {
		mw.panicCallback = callback
	}
}
