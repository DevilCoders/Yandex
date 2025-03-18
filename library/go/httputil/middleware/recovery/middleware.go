package recovery

import (
	"fmt"
	"net/http"

	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/nop"
)

type middleware struct {
	l             log.Structured
	panicCallback func(http.ResponseWriter, *http.Request, error)
}

// New returns a middleware that recovers from panics.
func New(opts ...MiddlewareOpt) func(http.Handler) http.Handler {
	mw := middleware{
		l: new(nop.Logger),
	}

	for _, opt := range opts {
		opt(&mw)
	}

	return mw.wrap
}

func (mw middleware) wrap(next http.Handler) http.Handler {
	fn := func(w http.ResponseWriter, r *http.Request) {
		defer func() {
			rv := recover()
			if rv == nil {
				return
			}

			err, ok := rv.(error)
			if !ok {
				err = fmt.Errorf("%+v", rv)
			}

			mw.l.Error("panic recovered", log.Error(err))

			if mw.panicCallback != nil {
				mw.panicCallback(w, r, err)
			}
		}()

		next.ServeHTTP(w, r)
	}

	return http.HandlerFunc(fn)
}
