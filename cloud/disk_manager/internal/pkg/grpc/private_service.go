package grpc

import (
	"context"
	"math"

	"github.com/golang/protobuf/ptypes/empty"
	"google.golang.org/grpc"

	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/operation"
	"a.yandex-team.ru/cloud/disk_manager/internal/api"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/clients/nbs"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/resources"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/pools"
	pools_protos "a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/pools/protos"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/errors"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/types"
)

////////////////////////////////////////////////////////////////////////////////

type privateService struct {
	taskScheduler   tasks.Scheduler
	nbsFactory      nbs.Factory
	poolService     pools.Service
	resourceStorage resources.Storage
}

func (s *privateService) ScheduleBlankOperation(
	ctx context.Context,
	req *empty.Empty,
) (*operation.Operation, error) {

	taskID, err := s.taskScheduler.ScheduleBlankTask(ctx)
	if err != nil {
		return nil, err
	}

	return s.taskScheduler.GetOperationProto(ctx, taskID)
}

func (s *privateService) RebaseOverlayDisk(
	ctx context.Context,
	req *api.RebaseOverlayDiskRequest,
) (*operation.Operation, error) {

	taskID, err := s.poolService.RebaseOverlayDisk(
		ctx,
		&pools_protos.RebaseOverlayDiskRequest{
			OverlayDisk: &types.Disk{
				ZoneId: req.DiskId.ZoneId,
				DiskId: req.DiskId.DiskId,
			},
			BaseDiskId:       req.BaseDiskId,
			TargetBaseDiskId: req.TargetBaseDiskId,
			SlotGeneration:   req.SlotGeneration,
		},
	)
	if err != nil {
		return nil, err
	}

	return s.taskScheduler.GetOperationProto(ctx, taskID)
}

func (s *privateService) RetireBaseDisk(
	ctx context.Context,
	req *api.RetireBaseDiskRequest,
) (*operation.Operation, error) {

	taskID, err := s.poolService.RetireBaseDisk(
		ctx,
		&pools_protos.RetireBaseDiskRequest{
			BaseDiskId: req.BaseDiskId,
			SrcDisk: &types.Disk{
				ZoneId: req.SrcDiskId.ZoneId,
				DiskId: req.SrcDiskId.DiskId,
			},
			SrcDiskCheckpointId: req.SrcDiskCheckpointId,
		},
	)
	if err != nil {
		return nil, err
	}

	return s.taskScheduler.GetOperationProto(ctx, taskID)
}

func (s *privateService) RetireBaseDisks(
	ctx context.Context,
	req *api.RetireBaseDisksRequest,
) (*operation.Operation, error) {

	taskID, err := s.poolService.RetireBaseDisks(
		ctx,
		&pools_protos.RetireBaseDisksRequest{
			ImageId:          req.ImageId,
			ZoneId:           req.ZoneId,
			UseBaseDiskAsSrc: req.UseBaseDiskAsSrc,
		},
	)
	if err != nil {
		return nil, err
	}

	return s.taskScheduler.GetOperationProto(ctx, taskID)
}

func (s *privateService) OptimizeBaseDisks(
	ctx context.Context,
	req *empty.Empty,
) (*operation.Operation, error) {

	taskID, err := s.poolService.OptimizeBaseDisks(ctx)
	if err != nil {
		return nil, err
	}

	return s.taskScheduler.GetOperationProto(ctx, taskID)
}

func (s *privateService) ConfigurePool(
	ctx context.Context,
	req *api.ConfigurePoolRequest,
) (*operation.Operation, error) {

	// NBS-1375.
	if !s.nbsFactory.HasClient(req.ZoneId) {
		return nil, errors.CreateInvalidArgumentError(
			"unknown zone id: %v",
			req.ZoneId,
		)
	}

	if req.Capacity < 0 || req.Capacity > math.MaxUint32 {
		return nil, errors.CreateInvalidArgumentError(
			"invalid capacity: %v",
			req.Capacity,
		)
	}

	taskID, err := s.poolService.ConfigurePool(
		ctx,
		&pools_protos.ConfigurePoolRequest{
			ImageId:      req.ImageId,
			ZoneId:       req.ZoneId,
			Capacity:     uint32(req.Capacity),
			UseImageSize: req.UseImageSize,
		},
	)
	if err != nil {
		return nil, err
	}

	return s.taskScheduler.GetOperationProto(ctx, taskID)
}

func (s *privateService) DeletePool(
	ctx context.Context,
	req *api.DeletePoolRequest,
) (*operation.Operation, error) {

	taskID, err := s.poolService.DeletePool(
		ctx,
		&pools_protos.DeletePoolRequest{
			ImageId: req.ImageId,
			ZoneId:  req.ZoneId,
		},
	)
	if err != nil {
		return nil, err
	}

	return s.taskScheduler.GetOperationProto(ctx, taskID)
}

func (s *privateService) ListDisks(
	ctx context.Context,
	req *api.ListDisksRequest,
) (*api.ListDisksResponse, error) {

	ids, err := s.resourceStorage.ListDisks(ctx, req.FolderId)
	if err != nil {
		return nil, err
	}

	return &api.ListDisksResponse{DiskIds: ids}, nil
}

func (s *privateService) ListImages(
	ctx context.Context,
	req *api.ListImagesRequest,
) (*api.ListImagesResponse, error) {

	ids, err := s.resourceStorage.ListImages(ctx, req.FolderId)
	if err != nil {
		return nil, err
	}

	return &api.ListImagesResponse{ImageIds: ids}, nil
}

func (s *privateService) ListSnapshots(
	ctx context.Context,
	req *api.ListSnapshotsRequest,
) (*api.ListSnapshotsResponse, error) {

	ids, err := s.resourceStorage.ListSnapshots(ctx, req.FolderId)
	if err != nil {
		return nil, err
	}

	return &api.ListSnapshotsResponse{SnapshotIds: ids}, nil
}

func (s *privateService) ListFilesystems(
	ctx context.Context,
	req *api.ListFilesystemsRequest,
) (*api.ListFilesystemsResponse, error) {

	ids, err := s.resourceStorage.ListFilesystems(ctx, req.FolderId)
	if err != nil {
		return nil, err
	}

	return &api.ListFilesystemsResponse{FilesystemIds: ids}, nil
}

func (s *privateService) ListPlacementGroups(
	ctx context.Context,
	req *api.ListPlacementGroupsRequest,
) (*api.ListPlacementGroupsResponse, error) {

	ids, err := s.resourceStorage.ListPlacementGroups(ctx, req.FolderId)
	if err != nil {
		return nil, err
	}

	return &api.ListPlacementGroupsResponse{PlacementGroupIds: ids}, nil
}

////////////////////////////////////////////////////////////////////////////////

func RegisterPrivateService(
	server *grpc.Server,
	taskScheduler tasks.Scheduler,
	nbsFactory nbs.Factory,
	poolService pools.Service,
	resourceStorage resources.Storage,
) {

	api.RegisterPrivateServiceServer(server, &privateService{
		taskScheduler:   taskScheduler,
		nbsFactory:      nbsFactory,
		poolService:     poolService,
		resourceStorage: resourceStorage,
	})
}
