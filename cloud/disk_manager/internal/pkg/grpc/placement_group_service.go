package grpc

import (
	"context"

	"google.golang.org/grpc"

	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/operation"
	"a.yandex-team.ru/cloud/disk_manager/api/yandex/cloud/priv/disk_manager/v1"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/placementgroup"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks"
)

////////////////////////////////////////////////////////////////////////////////

type placementGroupService struct {
	scheduler tasks.Scheduler
	service   placementgroup.Service
}

func (s *placementGroupService) Create(
	ctx context.Context,
	req *disk_manager.CreatePlacementGroupRequest,
) (*operation.Operation, error) {

	taskID, err := s.service.CreatePlacementGroup(ctx, req)
	if err != nil {
		return nil, err
	}

	return s.scheduler.GetOperationProto(ctx, taskID)
}

func (s *placementGroupService) Delete(
	ctx context.Context,
	req *disk_manager.DeletePlacementGroupRequest,
) (*operation.Operation, error) {

	taskID, err := s.service.DeletePlacementGroup(ctx, req)
	if err != nil {
		return nil, err
	}

	return s.scheduler.GetOperationProto(ctx, taskID)
}

func (s *placementGroupService) Alter(
	ctx context.Context,
	req *disk_manager.AlterPlacementGroupMembershipRequest,
) (*operation.Operation, error) {

	taskID, err := s.service.AlterPlacementGroupMembership(ctx, req)
	if err != nil {
		return nil, err
	}

	return s.scheduler.GetOperationProto(ctx, taskID)
}

func (s *placementGroupService) List(
	ctx context.Context,
	req *disk_manager.ListPlacementGroupsRequest,
) (*disk_manager.ListPlacementGroupsResponse, error) {

	return s.service.ListPlacementGroups(ctx, req)
}

func (s *placementGroupService) Describe(
	ctx context.Context,
	req *disk_manager.DescribePlacementGroupRequest,
) (*disk_manager.PlacementGroup, error) {

	return s.service.DescribePlacementGroup(ctx, req)
}

////////////////////////////////////////////////////////////////////////////////

func RegisterPlacementGroupService(
	server *grpc.Server,
	scheduler tasks.Scheduler,
	service placementgroup.Service,
) {

	disk_manager.RegisterPlacementGroupServiceServer(server, &placementGroupService{
		scheduler: scheduler,
		service:   service,
	})
}
