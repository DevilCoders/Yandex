package instanceservice

import (
	"context"

	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/marketplace/licenseserver/v1/license"
	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/marketplace/licenseserver/v1/license/priv"
	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/operation"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/actions/license/instances"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/actions/operations"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/model"
	operations_grpc "a.yandex-team.ru/cloud/marketplace/license_server/internal/services/grpc/operations"
)

func (s *instanceService) Cancel(ctx context.Context, request *priv.CancelInstanceRequest) (*operation.Operation, error) {
	op, err := operations.NewCreateAction(s.Env).Do(ctx, operations.CreateParams{Description: model.CancelLicenseInstanceDescription})
	if err := s.MapActionError(ctx, err); err != nil {
		return nil, err
	}

	result, opErr := instances.NewCancelAction(s.Env).Do(ctx, instances.CancelParams{ID: request.Id})
	opErr = s.MapActionError(ctx, opErr)

	meta := &priv.CancelInstanceMetadata{
		InstanceId: request.Id,
	}
	var res *license.Instance
	if opErr == nil {
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
