package restapi

import (
	"fmt"
	"net"

	"a.yandex-team.ru/cloud/mdb/internal/crt/cloudcrt/generated/swagger/models"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func apiError(err error) error {
	if err == nil {
		return err
	}
	switch err := err.(type) {
	case swaggerError:
		code := err.Code()
		msg := fmt.Sprintf("[%d] %s", err.Code(), err.GetPayload().Message)
		if code == 500 {
			return semerr.Internal(msg)
		} else if code > 500 {
			return semerr.Unavailable(msg)
		} else if code == 401 {
			return semerr.Authentication(msg)
		} else if code == 403 {
			return semerr.Authorization(msg)
		} else if code == 404 {
			return semerr.NotFound(msg)
		}
		return xerrors.Errorf(msg)
	case net.Error:
		return semerr.WrapWithUnavailable(err, "cloud cert")
	default:
		return err
	}
}

type swaggerError interface {
	Code() int
	GetPayload() *models.RequestsErrorResponse
}
