package grpc

import (
	"context"

	"google.golang.org/grpc"

	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/operation"
	"a.yandex-team.ru/cloud/disk_manager/api/yandex/cloud/priv/disk_manager/v1"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/filesystem"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks"
)

////////////////////////////////////////////////////////////////////////////////

type filesystemService struct {
	scheduler tasks.Scheduler
	service   filesystem.Service
}

func (s *filesystemService) Create(
	ctx context.Context,
	req *disk_manager.CreateFilesystemRequest,
) (*operation.Operation, error) {

	taskID, err := s.service.CreateFilesystem(ctx, req)
	if err != nil {
		return nil, err
	}

	return s.scheduler.GetOperationProto(ctx, taskID)
}

func (s *filesystemService) Delete(
	ctx context.Context,
	req *disk_manager.DeleteFilesystemRequest,
) (*operation.Operation, error) {

	taskID, err := s.service.DeleteFilesystem(ctx, req)
	if err != nil {
		return nil, err
	}

	return s.scheduler.GetOperationProto(ctx, taskID)
}

func (s *filesystemService) Resize(
	ctx context.Context,
	req *disk_manager.ResizeFilesystemRequest,
) (*operation.Operation, error) {

	taskID, err := s.service.ResizeFilesystem(ctx, req)
	if err != nil {
		return nil, err
	}

	return s.scheduler.GetOperationProto(ctx, taskID)
}

func (s *filesystemService) DescribeModel(
	ctx context.Context,
	req *disk_manager.DescribeFilesystemModelRequest,
) (*disk_manager.FilesystemModel, error) {

	return s.service.DescribeFilesystemModel(ctx, req)
}

////////////////////////////////////////////////////////////////////////////////

func RegisterFilesystemService(
	server *grpc.Server,
	scheduler tasks.Scheduler,
	service filesystem.Service,
) {

	disk_manager.RegisterFilesystemServiceServer(server, &filesystemService{
		scheduler: scheduler,
		service:   service,
	})
}
