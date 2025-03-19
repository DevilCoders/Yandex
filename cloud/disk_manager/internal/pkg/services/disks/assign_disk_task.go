package disks

import (
	"context"
	"fmt"

	"github.com/golang/protobuf/proto"
	"github.com/golang/protobuf/ptypes/empty"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/clients/nbs"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/disks/protos"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks"
)

////////////////////////////////////////////////////////////////////////////////

type assignDiskTask struct {
	nbsFactory nbs.Factory
	state      *protos.AssignDiskTaskState
}

func (t *assignDiskTask) Init(
	ctx context.Context,
	request proto.Message,
) error {

	typedRequest, ok := request.(*protos.AssignDiskRequest)
	if !ok {
		return fmt.Errorf("invalid request=%v type", request)
	}

	t.state = &protos.AssignDiskTaskState{
		Request: typedRequest,
	}

	return nil
}

func (t *assignDiskTask) Save(ctx context.Context) ([]byte, error) {
	return proto.Marshal(t.state)
}

func (t *assignDiskTask) Load(ctx context.Context, state []byte) error {
	t.state = &protos.AssignDiskTaskState{}
	return proto.Unmarshal(state, t.state)
}

func (t *assignDiskTask) Run(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
) error {

	request := t.state.Request

	client, err := t.nbsFactory.GetClient(ctx, request.Disk.ZoneId)
	if err != nil {
		return err
	}

	return client.Assign(ctx, nbs.AssignDiskParams{
		ID:         request.Disk.DiskId,
		InstanceID: request.InstanceId,
		Host:       request.Host,
		Token:      request.Token,
	})
}

func (t *assignDiskTask) Cancel(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
) error {

	// TODO: should it be cancellable?
	// TODO: should we do unassign?
	return nil
}

func (t *assignDiskTask) GetMetadata(
	ctx context.Context,
) (proto.Message, error) {

	return &empty.Empty{}, nil
}

func (t *assignDiskTask) GetResponse() proto.Message {
	return &empty.Empty{}
}
