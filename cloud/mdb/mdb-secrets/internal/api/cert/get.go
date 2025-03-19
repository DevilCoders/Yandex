package cert

import (
	"context"

	"github.com/go-openapi/runtime/middleware"
	"github.com/go-openapi/strfmt"

	"a.yandex-team.ru/cloud/mdb/internal/auth/httpauth"
	"a.yandex-team.ru/cloud/mdb/mdb-secrets/generated/swagger/models"
	"a.yandex-team.ru/cloud/mdb/mdb-secrets/generated/swagger/restapi/operations/certs"
	"a.yandex-team.ru/cloud/mdb/mdb-secrets/internal/secretsdb"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (s *Service) GetCertHandler(ctx context.Context, params certs.GetCertificateParams) middleware.Responder {
	if err := s.auth.Auth(ctx, params.HTTPRequest); err != nil {
		switch {
		case xerrors.Is(err, httpauth.ErrAuthFailure):
			return certs.NewGetCertificateUnauthorized().WithPayload(&models.Error{Message: err.Error()})
		case xerrors.Is(err, httpauth.ErrAuthNoRights):
			return certs.NewGetCertificateForbidden().WithPayload(&models.Error{Message: err.Error()})
		default:
			return certs.NewGetCertificateInternalServerError().WithPayload(&models.Error{
				Message: err.Error(),
			})
		}
	}

	cert, err := s.secretsDB.GetCert(ctx, params.Hostname)

	if err != nil {
		if xerrors.Is(err, secretsdb.ErrCertNotFound) {
			return certs.NewGetCertificateNotFound()
		}
		if xerrors.Is(err, secretsdb.ErrNotAvailable) || xerrors.Is(err, secretsdb.ErrNoMaster) {
			return certs.NewGetCertificateServiceUnavailable().WithPayload(&models.Error{
				Message: err.Error(),
			})
		}
		return certs.NewGetCertificateInternalServerError().WithPayload(&models.Error{
			Message: err.Error(),
		})
	}

	return certs.NewGetCertificateOK().WithPayload(&models.CertResponse{
		Cert: cert.Crt,
		Key: &models.EncryptedMessage{
			Data:    cert.Key.Data,
			Version: cert.Key.Version,
		},
		Expiration: strfmt.DateTime(cert.Expiration),
	})
}
