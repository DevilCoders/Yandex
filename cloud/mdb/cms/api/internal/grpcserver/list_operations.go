package grpcserver

import (
	"context"

	api "a.yandex-team.ru/cloud/mdb/cms/api/grpcapi/v1"
)

func (s *InstanceService) List(ctx context.Context, _ *api.ListInstanceOperationsRequest) (*api.ListInstanceOperationsResponse, error) {
	ops, err := s.cmsdb.ListInstanceOperations(ctx)
	if err != nil {
		return nil, err
	}
	res := make([]*api.InstanceOperation, len(ops))
	for ind, op := range ops {
		grpcOp, err := internalOperationToGRPC(op, s.log)
		if err != nil {
			return nil, err
		}
		res[ind] = grpcOp
	}
	return &api.ListInstanceOperationsResponse{Operations: res}, err
}
