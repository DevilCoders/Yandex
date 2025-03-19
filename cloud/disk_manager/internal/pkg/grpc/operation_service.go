package grpc

import (
	"context"

	"google.golang.org/grpc"

	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/operation"
	"a.yandex-team.ru/cloud/disk_manager/api/yandex/cloud/priv/disk_manager/v1"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks"
)

////////////////////////////////////////////////////////////////////////////////

type operationService struct {
	taskScheduler tasks.Scheduler
}

func (s *operationService) Get(
	ctx context.Context,
	req *disk_manager.GetOperationRequest,
) (*operation.Operation, error) {

	return s.taskScheduler.GetOperationProto(ctx, req.OperationId)
}

func (s *operationService) Cancel(
	ctx context.Context,
	req *disk_manager.CancelOperationRequest,
) (*operation.Operation, error) {

	_, err := s.taskScheduler.CancelTask(ctx, req.OperationId)
	if err != nil {
		return nil, err
	}

	return s.taskScheduler.GetOperationProto(ctx, req.OperationId)
}

////////////////////////////////////////////////////////////////////////////////

func RegisterOperationService(
	server *grpc.Server,
	taskScheduler tasks.Scheduler,
) {

	disk_manager.RegisterOperationServiceServer(server, &operationService{
		taskScheduler: taskScheduler,
	})
}
