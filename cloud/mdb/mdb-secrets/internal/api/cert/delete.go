package cert

import (
	"context"
	"net/http"

	"github.com/go-openapi/runtime/middleware"

	"a.yandex-team.ru/cloud/mdb/internal/auth/httpauth"
	"a.yandex-team.ru/cloud/mdb/mdb-secrets/generated/swagger/models"
	"a.yandex-team.ru/cloud/mdb/mdb-secrets/generated/swagger/restapi/operations/certs"
	"a.yandex-team.ru/cloud/mdb/mdb-secrets/internal/secretsdb"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func handleDeleteErr(status int, msg string) *certs.RevokeCertificateDefault {
	return certs.NewRevokeCertificateDefault(status).
		WithPayload(&models.Error{Message: msg})
}

// Delete certificate if it exists.
func (s *Service) DeleteCertificate(ctx context.Context, params certs.RevokeCertificateParams) middleware.Responder {
	ctx = ctxlog.WithFields(ctx, log.String("secrets action", "revoke_certificate"))
	if err := s.auth.Auth(ctx, params.HTTPRequest); err != nil {
		return handleDeleteErr(mapAuthError(err), err.Error())
	}
	revokedCnt, err := s.crtClient.RevokeCertsByHostname(ctx, params.Hostname)
	if err != nil {
		ctxlog.Error(ctx, s.lg, "did not revoke all certificates", log.Error(err))
		return handleDeleteErr(http.StatusInternalServerError, err.Error())
	}
	_, err = s.secretsDB.GetCert(ctx, params.Hostname)
	if err != nil {
		ctxlog.Error(ctx, s.lg, "get certificate from db error", log.Error(err))
		if xerrors.Is(err, secretsdb.ErrCertNotFound) {
			return handleDeleteErr(http.StatusNotFound, "")
		}
	}
	err = s.secretsDB.DeleteCert(ctx, params.Hostname)
	if err != nil {
		ctxlog.Error(ctx, s.lg, "delete certificate from db error", log.Error(err))
		return handleDeleteErr(http.StatusInternalServerError, err.Error())
	}
	ctxlog.Infof(ctx, s.lg, "revoked %d certificates", revokedCnt)
	return certs.NewRevokeCertificateOK().WithPayload(&certs.RevokeCertificateOKBody{Message: "ok"})
}

func mapAuthError(err error) int {
	switch {
	case xerrors.Is(err, httpauth.ErrAuthFailure):
		return http.StatusUnauthorized
	case xerrors.Is(err, httpauth.ErrAuthNoRights):
		return http.StatusForbidden
	default:
		return http.StatusInternalServerError
	}
}
