package grpcserver

import (
	"context"

	api "a.yandex-team.ru/cloud/mdb/cms/api/grpcapi/v1"
	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/models"
	"a.yandex-team.ru/cloud/mdb/internal/idempotence"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
)

func (s *InstanceService) WhipPrimary(ctx context.Context, req *api.WhipPrimaryRequest) (*api.InstanceOperation, error) {
	idem, ok := idempotence.IncomingFromContext(ctx)
	if !ok {
		return nil, semerr.InvalidInput("idempotence key must be specified")
	}

	author, err := s.authorize(ctx)
	if err != nil {
		return nil, err
	}

	_, err = validateInstanceID(ctx, req.InstanceId, s.mdb, s.cfg.IsCompute)
	if err != nil {
		return nil, err
	}

	opID, err := s.cmsdb.CreateInstanceOperation(
		ctx,
		idem.ID,
		models.InstanceOperationWhipPrimaryAway,
		req.InstanceId,
		req.Comment,
		author,
	)
	if err != nil {
		return nil, err
	}

	return s.Get(ctx, &api.GetInstanceOperationRequest{Id: opID})
}
