package swagapi

import (
	"net/http"
	"net/url"

	"github.com/go-openapi/runtime"
	"github.com/go-openapi/runtime/middleware"

	swagmodels "a.yandex-team.ru/cloud/mdb/deploy/api/generated/swagger/models"
	"a.yandex-team.ru/cloud/mdb/deploy/api/internal/core"
	"a.yandex-team.ru/cloud/mdb/internal/httputil"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type errorResponder struct {
	Code    int
	Payload *swagmodels.Error
}

func (er *errorResponder) WriteResponse(rw http.ResponseWriter, producer runtime.Producer) {
	rw.WriteHeader(er.Code)
	if er.Payload != nil {
		payload := er.Payload
		if err := producer.Produce(rw, payload); err != nil {
			panic(err) // let the recovery middleware deal with this
		}
	}
}

func newAuthError(u *url.URL, err error, lg log.Logger) middleware.Responder {
	lg.Warnf("Responding with error to %q request: %s", u, err)

	payload := &swagmodels.Error{Message: err.Error()}
	switch {
	case xerrors.Is(err, core.ErrAuthFailure) || xerrors.Is(err, core.ErrAuthTokenEmpty):
		return &errorResponder{Code: http.StatusUnauthorized, Payload: payload}
	case xerrors.Is(err, core.ErrAuthTemporaryFailure):
		return &errorResponder{Code: http.StatusBadGateway, Payload: payload}
	case xerrors.Is(err, core.ErrAuthNoRights):
		fallthrough
	default:
		return &errorResponder{Code: http.StatusForbidden, Payload: payload}
	}
}

type swagError interface {
	middleware.Responder

	SetStatusCode(code int)
	SetPayload(payload *swagmodels.Error)
}

func newLogicError(se swagError, err error) swagError {
	code, ok := httputil.CodeFromSemanticError(err)
	if !ok {
		code = http.StatusInternalServerError
	}

	se.SetStatusCode(code)
	se.SetPayload(&swagmodels.Error{Message: err.Error()})
	return se
}
