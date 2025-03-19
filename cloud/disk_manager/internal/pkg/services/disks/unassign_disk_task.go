package disks

import (
	"context"
	"fmt"

	"github.com/golang/protobuf/proto"
	"github.com/golang/protobuf/ptypes/empty"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/clients/nbs"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/disks/protos"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/errors"
)

////////////////////////////////////////////////////////////////////////////////

type unassignDiskTask struct {
	nbsFactory nbs.Factory
	state      *protos.UnassignDiskTaskState
}

func (t *unassignDiskTask) Init(
	ctx context.Context,
	request proto.Message,
) error {

	typedRequest, ok := request.(*protos.UnassignDiskRequest)
	if !ok {
		return fmt.Errorf("invalid request=%v type", request)
	}

	t.state = &protos.UnassignDiskTaskState{
		Request: typedRequest,
	}

	return nil
}

func (t *unassignDiskTask) Save(ctx context.Context) ([]byte, error) {
	return proto.Marshal(t.state)
}

func (t *unassignDiskTask) Load(ctx context.Context, state []byte) error {
	t.state = &protos.UnassignDiskTaskState{}
	return proto.Unmarshal(state, t.state)
}

func (t *unassignDiskTask) unassign(
	ctx context.Context,
) error {

	request := t.state.Request

	client, err := t.nbsFactory.GetClient(ctx, request.Disk.ZoneId)
	if err != nil {
		return err
	}

	return client.Unassign(ctx, request.Disk.DiskId)
}

func (t *unassignDiskTask) Run(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
) error {

	err := t.unassign(ctx)
	return errors.MakeRetriable(err, true /* ignoreRetryLimit */)
}

func (t *unassignDiskTask) Cancel(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
) error {

	return t.unassign(ctx)
}

func (t *unassignDiskTask) GetMetadata(
	ctx context.Context,
) (proto.Message, error) {

	return &empty.Empty{}, nil
}

func (t *unassignDiskTask) GetResponse() proto.Message {
	return &empty.Empty{}
}
