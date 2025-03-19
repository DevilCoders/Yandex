package placementgroup

import (
	"context"
	"fmt"
	"time"

	"github.com/golang/protobuf/proto"
	"github.com/golang/protobuf/ptypes/empty"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/clients/nbs"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/resources"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/placementgroup/protos"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/errors"
)

////////////////////////////////////////////////////////////////////////////////

type createPlacementGroupTask struct {
	storage    resources.Storage
	nbsFactory nbs.Factory
	state      *protos.CreatePlacementGroupTaskState
}

func (t *createPlacementGroupTask) Init(
	ctx context.Context,
	request proto.Message,
) error {

	typedRequest, ok := request.(*protos.CreatePlacementGroupRequest)
	if !ok {
		return fmt.Errorf("invalid request=%v type", request)
	}

	t.state = &protos.CreatePlacementGroupTaskState{
		Request: typedRequest,
	}
	return nil
}

func (t *createPlacementGroupTask) Save(ctx context.Context) ([]byte, error) {
	return proto.Marshal(t.state)
}

func (t *createPlacementGroupTask) Load(ctx context.Context, state []byte) error {
	t.state = &protos.CreatePlacementGroupTaskState{}
	return proto.Unmarshal(state, t.state)
}

func (t *createPlacementGroupTask) Run(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
) error {

	request := t.state.Request

	client, err := t.nbsFactory.GetClient(ctx, request.ZoneId)
	if err != nil {
		return err
	}

	selfTaskID := execCtx.GetTaskID()

	placementGroupMeta, err := t.storage.CreatePlacementGroup(ctx, resources.PlacementGroupMeta{
		ID:                request.GroupId,
		ZoneID:            request.ZoneId,
		PlacementStrategy: request.PlacementStrategy,

		CreateRequest: request,
		CreateTaskID:  selfTaskID,
		CreatingAt:    time.Now(),
		CreatedBy:     "", // TODO: Extract CreatedBy from execCtx
	})
	if err != nil {
		return err
	}

	if placementGroupMeta == nil {
		return &errors.NonCancellableError{
			Err: fmt.Errorf("id=%v is not accepted", request.GroupId),
		}
	}

	err = client.CreatePlacementGroup(
		ctx,
		request.GroupId,
		request.PlacementStrategy,
	)
	if err != nil {
		return err
	}

	placementGroupMeta.CreatedAt = time.Now()
	return t.storage.PlacementGroupCreated(ctx, *placementGroupMeta)
}

func (t *createPlacementGroupTask) Cancel(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
) error {

	request := t.state.Request

	client, err := t.nbsFactory.GetClient(ctx, request.ZoneId)
	if err != nil {
		return err
	}

	selfTaskID := execCtx.GetTaskID()

	placementGroupMeta, err := t.storage.DeletePlacementGroup(
		ctx,
		request.GroupId,
		selfTaskID,
		time.Now(),
	)
	if err != nil {
		return err
	}

	if placementGroupMeta == nil {
		return &errors.NonCancellableError{
			Err: fmt.Errorf("id=%v is not accepted", request.GroupId),
		}
	}

	err = client.DeletePlacementGroup(ctx, request.GroupId)
	if err != nil {
		return err
	}

	return t.storage.PlacementGroupDeleted(ctx, request.GroupId, time.Now())
}

func (t *createPlacementGroupTask) GetMetadata(
	ctx context.Context,
) (proto.Message, error) {

	return &empty.Empty{}, nil
}

func (t *createPlacementGroupTask) GetResponse() proto.Message {
	return &empty.Empty{}
}
