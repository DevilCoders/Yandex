package templateversionservice

import (
	"context"

	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/marketplace/licenseserver/v1/license/priv"
	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/operation"
	templateversions "a.yandex-team.ru/cloud/marketplace/license_server/internal/core/actions/license/template_versions"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/actions/operations"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/model"
	operations_grpc "a.yandex-team.ru/cloud/marketplace/license_server/internal/services/grpc/operations"
	"a.yandex-team.ru/cloud/marketplace/pkg/protob"
)

func (s *templateVersionService) Delete(ctx context.Context, request *priv.DeleteTemplateVersionRequest) (*operation.Operation, error) {
	op, err := operations.NewCreateAction(s.Env).Do(ctx, operations.CreateParams{Description: model.DeleteLicenseTemplateVersionDescription})
	if err := s.MapActionError(ctx, err); err != nil {
		return nil, err
	}

	opErr := templateversions.NewDeleteAction(s.Env).Do(ctx, templateversions.DeleteParams{ID: request.Id})
	opErr = s.MapActionError(ctx, opErr)

	meta := &priv.DeleteTemplateVersionMetadata{
		TemplateVersionId: request.Id,
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
