package grpc

import (
	"context"

	"google.golang.org/grpc"

	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/operation"
	"a.yandex-team.ru/cloud/disk_manager/api/yandex/cloud/priv/disk_manager/v1"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/snapshots"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks"
)

////////////////////////////////////////////////////////////////////////////////

type snapshotService struct {
	scheduler tasks.Scheduler
	service   snapshots.Service
}

func (s *snapshotService) Create(
	ctx context.Context,
	req *disk_manager.CreateSnapshotRequest,
) (*operation.Operation, error) {

	taskID, err := s.service.CreateSnapshot(ctx, req)
	if err != nil {
		return nil, err
	}

	return s.scheduler.GetOperationProto(ctx, taskID)
}

func (s *snapshotService) Delete(
	ctx context.Context,
	req *disk_manager.DeleteSnapshotRequest,
) (*operation.Operation, error) {

	taskID, err := s.service.DeleteSnapshot(ctx, req)
	if err != nil {
		return nil, err
	}

	return s.scheduler.GetOperationProto(ctx, taskID)
}

func (s *snapshotService) RestoreDisk(
	ctx context.Context,
	req *disk_manager.RestoreDiskFromSnapshotRequest,
) (*operation.Operation, error) {

	taskID, err := s.service.RestoreDiskFromSnapshot(ctx, req)
	if err != nil {
		return nil, err
	}

	return s.scheduler.GetOperationProto(ctx, taskID)
}

////////////////////////////////////////////////////////////////////////////////

func RegisterSnapshotService(
	server *grpc.Server,
	scheduler tasks.Scheduler,
	service snapshots.Service,
) {

	disk_manager.RegisterSnapshotServiceServer(server, &snapshotService{
		scheduler: scheduler,
		service:   service,
	})
}
