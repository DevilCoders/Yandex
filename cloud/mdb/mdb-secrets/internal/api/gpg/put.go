package gpg

import (
	"context"

	"github.com/go-openapi/runtime/middleware"

	"a.yandex-team.ru/cloud/mdb/internal/auth/httpauth"
	"a.yandex-team.ru/cloud/mdb/mdb-secrets/generated/swagger/models"
	apiGpg "a.yandex-team.ru/cloud/mdb/mdb-secrets/generated/swagger/restapi/operations/gpg"
	"a.yandex-team.ru/cloud/mdb/mdb-secrets/internal/gpg"
	"a.yandex-team.ru/cloud/mdb/mdb-secrets/internal/secretsdb"
	"a.yandex-team.ru/library/go/core/xerrors"
)

// PutGpgHandler PUT /v1/gpg/:cid
func (s *Service) PutGpgHandler(ctx context.Context, args apiGpg.GetParams) middleware.Responder {
	if err := s.auth.Auth(ctx, args.HTTPRequest); err != nil {
		switch {
		case xerrors.Is(err, httpauth.ErrAuthFailure):
			return apiGpg.NewGetUnauthorized().WithPayload(&models.Error{Message: err.Error()})
		case xerrors.Is(err, httpauth.ErrAuthNoRights):
			return apiGpg.NewGetForbidden().WithPayload(&models.Error{Message: err.Error()})
		default:
			return apiGpg.NewGetInternalServerError().WithPayload(&models.Error{
				Message: err.Error(),
			})
		}
	}

	key, err := s.secretsDB.GetGpg(ctx, args.Cid)
	switch {
	case xerrors.Is(err, secretsdb.ErrKeyNotFound):
		genKey, genErr := gpg.GenerateGPG(args.Cid)
		if genErr != nil {
			return apiGpg.NewGetInternalServerError().WithPayload(&models.Error{
				Message: genErr.Error(),
			})
		}
		key, err = s.secretsDB.PutGpg(ctx, args.Cid, genKey)
		if err != nil {
			return apiGpg.NewGetInternalServerError().WithPayload(&models.Error{
				Message: err.Error(),
			})
		}
	case err != nil:
		return apiGpg.NewGetInternalServerError().WithPayload(&models.Error{
			Message: err.Error(),
		})
	}

	return apiGpg.NewGetOK().WithPayload(&models.GpgKeyResponse{Key: &models.EncryptedMessage{
		Data:    key.Data,
		Version: key.Version,
	}})
}
