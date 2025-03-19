package placementgroup

import (
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/resources"
	"context"
	"fmt"
	"time"

	"github.com/golang/protobuf/proto"
	"github.com/golang/protobuf/ptypes/empty"

	"a.yandex-team.ru/cloud/disk_manager/api/yandex/cloud/priv/disk_manager/v1"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/clients/nbs"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/placementgroup/protos"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/errors"
)

////////////////////////////////////////////////////////////////////////////////

type deletePlacementGroupTask struct {
	storage    resources.Storage
	nbsFactory nbs.Factory
	state      *protos.DeletePlacementGroupTaskState
}

func (t *deletePlacementGroupTask) Init(
	ctx context.Context,
	request proto.Message,
) error {

	typedRequest, ok := request.(*protos.DeletePlacementGroupRequest)
	if !ok {
		return fmt.Errorf("invalid request=%v type", request)
	}

	t.state = &protos.DeletePlacementGroupTaskState{
		Request: typedRequest,
	}
	return nil
}

func (t *deletePlacementGroupTask) Save(ctx context.Context) ([]byte, error) {
	return proto.Marshal(t.state)
}

func (t *deletePlacementGroupTask) Load(ctx context.Context, state []byte) error {
	t.state = &protos.DeletePlacementGroupTaskState{}
	return proto.Unmarshal(state, t.state)
}

func (t *deletePlacementGroupTask) deletePlacementGroup(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
) error {

	request := t.state.Request

	client, err := t.nbsFactory.GetClient(ctx, request.ZoneId)
	if err != nil {
		return err
	}

	selfTaskID := execCtx.GetTaskID()

	placementGroup, err := t.storage.DeletePlacementGroup(
		ctx,
		request.GroupId,
		selfTaskID,
		time.Now(),
	)
	if err != nil {
		return err
	}

	if placementGroup == nil {
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

func (t *deletePlacementGroupTask) Run(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
) error {

	err := t.deletePlacementGroup(ctx, execCtx)
	return errors.MakeRetriable(err, true /* ignoreRetryLimit */)
}

func (t *deletePlacementGroupTask) Cancel(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
) error {

	return t.deletePlacementGroup(ctx, execCtx)
}

func (t *deletePlacementGroupTask) GetMetadata(
	ctx context.Context,
) (proto.Message, error) {

	return &disk_manager.DeletePlacementGroupMetadata{
		GroupId: &disk_manager.GroupId{
			ZoneId:  t.state.Request.ZoneId,
			GroupId: t.state.Request.GroupId,
		},
	}, nil
}

func (t *deletePlacementGroupTask) GetResponse() proto.Message {
	return &empty.Empty{}
}
