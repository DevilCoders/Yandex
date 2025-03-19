package instanceservice

import (
	"context"

	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/marketplace/licenseserver/v1/license"
	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/marketplace/licenseserver/v1/license/console"
	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/operation"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/actions/license/instances"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/actions/operations"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/model"
	operations_grpc "a.yandex-team.ru/cloud/marketplace/license_server/internal/services/grpc/operations"
)

func (s *instanceService) Create(ctx context.Context, request *console.CreateInstanceRequest) (*operation.Operation, error) {
	op, err := operations.NewCreateAction(s.Env).Do(ctx, operations.CreateParams{Description: model.CreateLicenseInstanceDescription})
	if err := s.MapActionError(ctx, err); err != nil {
		return nil, err
	}

	result, opErr := instances.NewCreateAction(s.Env).Do(ctx, instances.CreateParams{TemplateID: request.TemplateId, CloudID: request.CloudId, Name: request.Name})
	opErr = s.MapActionError(ctx, opErr)

	var meta *console.CreateInstanceMetadata
	var res *license.Instance
	if opErr == nil {
		meta = &console.CreateInstanceMetadata{InstanceId: result.Instance.ID}
		res, err = result.Instance.Proto()
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
