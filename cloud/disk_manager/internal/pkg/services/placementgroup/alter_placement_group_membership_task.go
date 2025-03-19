package placementgroup

import (
	"context"
	"fmt"

	"github.com/golang/protobuf/proto"
	"github.com/golang/protobuf/ptypes/empty"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/clients/nbs"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/placementgroup/protos"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks"
)

////////////////////////////////////////////////////////////////////////////////

type alterPlacementGroupMembershipTask struct {
	nbsFactory nbs.Factory
	state      *protos.AlterPlacementGroupMembershipTaskState
}

func (t *alterPlacementGroupMembershipTask) Init(
	ctx context.Context,
	request proto.Message,
) error {

	typedRequest, ok := request.(*protos.AlterPlacementGroupMembershipRequest)
	if !ok {
		return fmt.Errorf("invalid request=%v type", request)
	}

	t.state = &protos.AlterPlacementGroupMembershipTaskState{
		Request: typedRequest,
	}

	return nil
}

func (t *alterPlacementGroupMembershipTask) Save(ctx context.Context) ([]byte, error) {
	return proto.Marshal(t.state)
}

func (t *alterPlacementGroupMembershipTask) Load(ctx context.Context, state []byte) error {
	t.state = &protos.AlterPlacementGroupMembershipTaskState{}
	return proto.Unmarshal(state, t.state)
}

func (t *alterPlacementGroupMembershipTask) Run(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
) error {

	request := t.state.Request

	client, err := t.nbsFactory.GetClient(ctx, request.ZoneId)
	if err != nil {
		return err
	}

	return client.AlterPlacementGroupMembership(
		ctx,
		func() error {
			// Confirm that current generation is not obsolete.
			return execCtx.SaveState(ctx)
		},
		request.GroupId,
		request.DisksToAdd,
		request.DisksToRemove,
	)
}

func (t *alterPlacementGroupMembershipTask) Cancel(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
) error {

	// TODO: should it be cancellable?
	return nil
}

func (t *alterPlacementGroupMembershipTask) GetMetadata(
	ctx context.Context,
) (proto.Message, error) {

	return &empty.Empty{}, nil
}

func (t *alterPlacementGroupMembershipTask) GetResponse() proto.Message {
	return &empty.Empty{}
}
