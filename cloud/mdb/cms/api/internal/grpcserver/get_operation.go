package grpcserver

import (
	"context"

	api "a.yandex-team.ru/cloud/mdb/cms/api/grpcapi/v1"
)

func (s *InstanceService) Get(ctx context.Context, req *api.GetInstanceOperationRequest) (*api.InstanceOperation, error) {
	op, err := s.cmsdb.GetInstanceOperation(ctx, req.Id)
	if err != nil {
		return nil, err
	}

	return internalOperationToGRPC(op, s.log)
}
