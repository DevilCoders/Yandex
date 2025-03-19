package gpg

import (
	"context"

	"github.com/go-openapi/runtime/middleware"

	"a.yandex-team.ru/cloud/mdb/internal/auth/httpauth"
	"a.yandex-team.ru/cloud/mdb/mdb-secrets/generated/swagger/models"
	apiGpg "a.yandex-team.ru/cloud/mdb/mdb-secrets/generated/swagger/restapi/operations/gpg"
	"a.yandex-team.ru/library/go/core/xerrors"
)

// DeleteGpgHandler DELETE /v1/gpg/:cid
func (s *Service) DeleteGpgHandler(ctx context.Context, args apiGpg.DeleteParams) middleware.Responder {
	if err := s.auth.Auth(ctx, args.HTTPRequest); err != nil {
		switch {
		case xerrors.Is(err, httpauth.ErrAuthFailure):
			return apiGpg.NewDeleteUnauthorized().WithPayload(&models.Error{Message: err.Error()})
		case xerrors.Is(err, httpauth.ErrAuthNoRights):
			return apiGpg.NewDeleteForbidden().WithPayload(&models.Error{Message: err.Error()})
		default:
			return apiGpg.NewDeleteInternalServerError().WithPayload(&models.Error{
				Message: err.Error(),
			})
		}
	}

	if err := s.secretsDB.DeleteGpg(args.HTTPRequest.Context(), args.Cid); err != nil {
		return apiGpg.NewDeleteInternalServerError().WithPayload(&models.Error{
			Message: err.Error(),
		})
	}

	return apiGpg.NewDeleteOK()
}
