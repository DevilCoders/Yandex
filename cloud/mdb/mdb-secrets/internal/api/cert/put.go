package cert

import (
	"context"
	"time"

	"github.com/go-openapi/runtime/middleware"
	"github.com/go-openapi/strfmt"

	"a.yandex-team.ru/cloud/mdb/internal/auth/httpauth"
	"a.yandex-team.ru/cloud/mdb/internal/crt"
	"a.yandex-team.ru/cloud/mdb/mdb-secrets/generated/swagger/models"
	"a.yandex-team.ru/cloud/mdb/mdb-secrets/generated/swagger/restapi/operations/certs"
	"a.yandex-team.ru/cloud/mdb/mdb-secrets/internal/secretsdb"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/slices"
)

// 30 days
const expiringPeriod = time.Hour * 24 * 30

// PutCertHandler gets existing certificate for hostname. If not found or expired, fetches another.
func (s *Service) PutCertHandler(ctx context.Context, params certs.PutParams) middleware.Responder {
	if err := s.auth.Auth(ctx, params.HTTPRequest); err != nil {
		switch {
		case xerrors.Is(err, httpauth.ErrAuthFailure):
			return certs.NewPutUnauthorized().WithPayload(&models.Error{Message: err.Error()})
		case xerrors.Is(err, httpauth.ErrAuthNoRights):
			return certs.NewPutForbidden().WithPayload(&models.Error{Message: err.Error()})
		default:
			return certs.NewPutInternalServerError().WithPayload(&models.Error{
				Message: err.Error(),
			})
		}
	}

	force := params.Force != nil && *params.Force

	var cert *secretsdb.Cert = nil
	var err error

	cert, err = s.secretsDB.GetCert(ctx, params.Hostname)

	if err != nil {
		if xerrors.Is(err, secretsdb.ErrCertNotFound) {
			err = s.secretsDB.InsertDummyCert(ctx, params.Hostname)
			if err != nil {
				return wrapPutError(ctx, s.lg, xerrors.Errorf("InsertDummyCert: %w", err))
			}
		} else {
			return wrapPutError(ctx, s.lg, xerrors.Errorf("GetCert: %w", err))
		}
	}

	needUpdate := func(cert *secretsdb.Cert) bool {
		if force {
			return true
		}
		if cert == nil {
			return true
		}
		if s.certExpiring(cert) {
			return true
		}
		// don't issue new if new alt_names is a subset of old
		if len(params.AltNames) != len(slices.IntersectStrings(cert.AltNames, params.AltNames)) {
			return true
		}
		return false
	}

	if needUpdate(cert) {
		cert, err = s.secretsDB.UpdateCert(ctx,
			params.Hostname,
			params.Ca,
			func(ctx context.Context) (update *secretsdb.CertUpdate, err error) {
				u, err := s.newCert(ctx, params.Hostname, params.AltNames, params.Ca, crt.CertificateType(params.Type), force)
				if err != nil {
					return nil, xerrors.Errorf("newCert: %w", err)
				}
				return u, nil
			},
			needUpdate)
		if err != nil {
			return wrapPutError(ctx, s.lg, xerrors.Errorf("UpdateCert: %w", err))
		}
	}

	return certs.NewPutOK().WithPayload(&models.CertResponse{
		Cert: cert.Crt,
		Key: &models.EncryptedMessage{
			Data:    cert.Key.Data,
			Version: cert.Key.Version,
		},
		Expiration: strfmt.DateTime(cert.Expiration),
	})
}

func (s *Service) newCert(ctx context.Context, hostname string, altNames []string, caName string, certType crt.CertificateType, force bool) (*secretsdb.CertUpdate, error) {
	var crtCert *crt.Cert = nil
	var err error
	if force {
		crtCert, err = s.crtClient.IssueCert(ctx, hostname, altNames, caName, certType)
		if err != nil {
			return nil, xerrors.Errorf("IssueCert: %w", err)
		}
	} else {
		crtCert, err = s.crtClient.ExistingCert(ctx, hostname, caName, altNames)
		switch {
		case err == nil:
			break
		case xerrors.Is(err, crt.ErrNoCerts):
			crtCert, err = s.crtClient.IssueCert(ctx, hostname, altNames, caName, certType)
			if err != nil {
				return nil, xerrors.Errorf("IssueCert: %w", err)
			}
		default:
			return nil, xerrors.Errorf("ExistingCert: %w", err)
		}
	}

	return &secretsdb.CertUpdate{
		Key:        string(crtCert.KeyPem),
		Crt:        string(crtCert.CertPem),
		Expiration: crtCert.Expiration,
		AltNames:   altNames,
	}, nil
}

func wrapPutError(ctx context.Context, lg log.Logger, err error) middleware.Responder {
	ctxlog.Error(ctx, lg, "put cert", log.Error(err))
	return certs.NewPutInternalServerError().WithPayload(&models.Error{
		Message: err.Error(),
	})
}

func (s *Service) certExpiring(cert *secretsdb.Cert) bool {
	return cert.Expiration.Add(-expiringPeriod).Before(s.nowF())
}
