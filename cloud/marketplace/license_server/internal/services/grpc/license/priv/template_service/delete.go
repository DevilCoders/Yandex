package templateservice

import (
	"context"

	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/marketplace/licenseserver/v1/license/priv"
	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/operation"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/actions/license/templates"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/actions/operations"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/model"
	operations_grpc "a.yandex-team.ru/cloud/marketplace/license_server/internal/services/grpc/operations"
	"a.yandex-team.ru/cloud/marketplace/pkg/protob"
)

func (s *templateService) Delete(ctx context.Context, request *priv.DeleteTemplateRequest) (*operation.Operation, error) {
	op, err := operations.NewCreateAction(s.Env).Do(ctx, operations.CreateParams{Description: model.DeleteLicenseTemplateDescription})
	if err := s.MapActionError(ctx, err); err != nil {
		return nil, err
	}

	opErr := templates.NewDeleteAction(s.Env).Do(ctx, templates.DeleteParams{ID: request.Id})
	opErr = s.MapActionError(ctx, opErr)

	meta := &priv.DeleteTemplateMetadata{
		TemplateId: request.Id,
	}
	res := protob.EmptyPB

	opUpdateParams, err := operations_grpc.GetOperationUpdateParams(op.ID, meta, res, opErr)
	if err != nil {
		return nil, err
	}

	op, err = operations.NewUpdateAction(s.Env).Do(ctx, *opUpdateParams)

	if err := s.MapActionError(ctx, err); err != nil {
		return nil, err
	}

	return op.Proto()
}
