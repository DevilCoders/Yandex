package cmsclient

import (
	"net"

	"a.yandex-team.ru/cloud/mdb/cms/api/generated/swagger/models"
	"a.yandex-team.ru/library/go/core/xerrors"
)

var (
	ErrUnavailable  = xerrors.NewSentinel("unavailable")
	ErrClientError  = xerrors.NewSentinel("client error")
	ErrNotFound     = xerrors.NewSentinel("not found")
	ErrUnknown      = xerrors.NewSentinel("unknown")
	ErrUnauthorized = xerrors.NewSentinel("unauthorized")
)

func apiError(err error) error {
	var e *xerrors.Sentinel
	switch err := err.(type) {
	case swaggerError:
		code := err.Code()
		if code > 500 {
			e = ErrUnavailable
		} else if code == 401 || code == 403 {
			e = ErrUnauthorized
		} else if code == 404 {
			e = ErrNotFound
		} else {
			e = ErrClientError
		}
		return e.Wrap(xerrors.Errorf("[%d] %s", err.Code(), err.GetPayload().Message))
	case net.Error:
		e = ErrUnavailable
	default:
		e = ErrUnknown
	}

	return e.Wrap(err)
}

type swaggerError interface {
	Code() int
	GetPayload() *models.Error
}
