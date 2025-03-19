package templateversionservice

import (
	"context"

	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/marketplace/licenseserver/v1/license/priv"
	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/operation"
	templateversions "a.yandex-team.ru/cloud/marketplace/license_server/internal/core/actions/license/template_versions"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/actions/operations"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/model"
	operations_grpc "a.yandex-team.ru/cloud/marketplace/license_server/internal/services/grpc/operations"
)

func (s *templateVersionService) Apply(ctx context.Context, request *priv.ApplyTemplateVersionRequest) (*operation.Operation, error) {
	op, err := operations.NewCreateAction(s.Env).Do(ctx, operations.CreateParams{Description: model.ApplyLicenseTemplateVersionDescription})
	if err := s.MapActionError(ctx, err); err != nil {
		return nil, err
	}

	result, opErr := templateversions.NewApplyAction(s.Env).Do(ctx, templateversions.ApplyParams{ID: request.Id})
	opErr = s.MapActionError(ctx, opErr)

	meta := &priv.ApplyTemplateVersionMetadata{TemplateVersionId: request.Id}
	var res *priv.ApplyTemplateVersionResponse
	if opErr != nil {
		res = &priv.ApplyTemplateVersionResponse{}
		res.LicenseTemplate, err = result.Template.Proto()
		if err != nil {
			return nil, err
		}
		res.LicenseTemplateVersion, err = result.TemplateVersion.Proto()
		if err != nil {
			return nil, err
		}
	}

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
