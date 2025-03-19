package util

import (
	"net/http"

	"a.yandex-team.ru/cloud/mdb/internal/httputil"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/library/go/core/log"
)

type RenderError = func(w http.ResponseWriter, err error)

func NewErrorRenderer(exposeDetails bool, logger log.Logger) RenderError {
	return func(w http.ResponseWriter, err error) {
		var httpCode int
		var msg string
		e := semerr.AsSemanticError(err)
		if e != nil {
			logger.Infof("%+v", err)
			httpCode = httputil.SemanticErrorToHTTP(e.Semantic)
			msg = e.Message
		} else {
			// non-semantic error is treated as internal server error
			logger.Errorf("%+v", err)
			httpCode = http.StatusInternalServerError
			msg = http.StatusText(http.StatusInternalServerError)
		}

		if exposeDetails {
			http.Error(w, err.Error(), httpCode)
		} else {
			http.Error(w, msg, httpCode)
		}
	}
}
