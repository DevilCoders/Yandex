package httputil

import (
	"net/http"

	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func CodeFromSemanticError(err error) (int, bool) {
	e := semerr.AsSemanticError(err)
	if e == nil {
		return 0, false
	}

	return SemanticErrorToHTTP(e.Semantic), true
}

func SemanticErrorToHTTP(s semerr.Semantic) int {
	switch s {
	case semerr.SemanticInvalidInput:
		return http.StatusBadRequest
	case semerr.SemanticAuthentication:
		return http.StatusUnauthorized
	case semerr.SemanticAuthorization:
		return http.StatusForbidden
	case semerr.SemanticNotFound:
		return http.StatusNotFound
	case semerr.SemanticFailedPrecondition:
		return http.StatusPreconditionFailed
	case semerr.SemanticAlreadyExists:
		return http.StatusConflict
	case semerr.SemanticNotImplemented:
		return http.StatusNotImplemented
	case semerr.SemanticUnavailable:
		return http.StatusServiceUnavailable
	case semerr.SemanticUnknown:
		fallthrough
	case semerr.SemanticInternal:
		fallthrough
	default:
		return http.StatusInternalServerError
	}
}

func SemanticErrorFromHTTP(code int, msg string) error {
	switch code {
	case http.StatusBadRequest:
		return semerr.InvalidInput(msg)
	case http.StatusUnauthorized:
		return semerr.Authentication(msg)
	case http.StatusForbidden:
		return semerr.Authorization(msg)
	case http.StatusNotFound:
		return semerr.NotFound(msg)
	case http.StatusPreconditionFailed:
		return semerr.FailedPrecondition(msg)
	case http.StatusConflict:
		return semerr.AlreadyExists(msg)
	case http.StatusNotImplemented:
		return semerr.NotImplemented(msg)
	case http.StatusServiceUnavailable, http.StatusGatewayTimeout:
		return semerr.Unavailable(msg)
	case http.StatusInternalServerError:
		return semerr.Internal(msg)
	default:
		return xerrors.Errorf(msg)
	}
}
