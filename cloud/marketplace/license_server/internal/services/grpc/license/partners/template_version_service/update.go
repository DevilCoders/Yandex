package templateversionservice

import (
	"context"

	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/marketplace/licenseserver/v1/license"
	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/marketplace/licenseserver/v1/license/partners"
	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/operation"
	templateversions "a.yandex-team.ru/cloud/marketplace/license_server/internal/core/actions/license/template_versions"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/actions/operations"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/model"
	operations_grpc "a.yandex-team.ru/cloud/marketplace/license_server/internal/services/grpc/operations"
)

func (s *templateVersionService) Update(ctx context.Context, request *partners.UpdateTemplateVersionRequest) (*operation.Operation, error) {
	op, err := operations.NewCreateAction(s.Env).Do(ctx, operations.CreateParams{Description: model.CreateLicenseTemplateDescription})
	if err := s.MapActionError(ctx, err); err != nil {
		return nil, err
	}

	result, opErr := templateversions.NewUpdateAction(s.Env).Do(ctx, templateversions.UpdateParams{
		ID:     request.Id,
		Price:  request.Prices.CurrencyToValue,
		Period: request.Period,
		Name:   request.Name,
	})
	opErr = s.MapActionError(ctx, opErr)

	meta := &partners.UpdateTemplateVersionMetadata{TemplateVersionId: request.Id}
	var res *license.TemplateVersion
	if opErr != nil {
		meta.TemplateVersionId = result.TemplateVersion.ID
		res, err = result.TemplateVersion.Proto()
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
