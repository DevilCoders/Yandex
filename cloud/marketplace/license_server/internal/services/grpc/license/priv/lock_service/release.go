package lockservice

import (
	"context"

	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/marketplace/licenseserver/v1/license"
	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/marketplace/licenseserver/v1/license/priv"
	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/operation"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/actions/license/locks"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/actions/operations"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/model"
	operations_grpc "a.yandex-team.ru/cloud/marketplace/license_server/internal/services/grpc/operations"
)

func (s *lockService) Release(ctx context.Context, request *priv.ReleaseLockRequest) (*operation.Operation, error) {
	op, err := operations.NewCreateAction(s.Env).Do(ctx, operations.CreateParams{Description: model.ReleaseLicenseLockDescription})
	if err := s.MapActionError(ctx, err); err != nil {
		return nil, err
	}

	result, opErr := locks.NewReleaseAction(s.Env).Do(ctx, locks.ReleaseParams{InstanceID: request.InstanceId})
	opErr = s.MapActionError(ctx, opErr)

	var meta *priv.ReleaseLockMetadata
	var res *license.Lock
	if opErr == nil {
		meta = &priv.ReleaseLockMetadata{LockId: result.Lock.ID}
		res, err = result.Lock.Proto()
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
