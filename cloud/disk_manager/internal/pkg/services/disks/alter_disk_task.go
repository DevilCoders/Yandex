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

type alterDiskTask struct {
	nbsFactory nbs.Factory
	state      *protos.AlterDiskTaskState
}

func (t *alterDiskTask) Init(
	ctx context.Context,
	request proto.Message,
) error {

	typedRequest, ok := request.(*protos.AlterDiskRequest)
	if !ok {
		return fmt.Errorf("invalid request=%v type", request)
	}

	t.state = &protos.AlterDiskTaskState{
		Request: typedRequest,
	}

	return nil
}

func (t *alterDiskTask) Save(ctx context.Context) ([]byte, error) {
	return proto.Marshal(t.state)
}

func (t *alterDiskTask) Load(ctx context.Context, state []byte) error {
	t.state = &protos.AlterDiskTaskState{}
	return proto.Unmarshal(state, t.state)
}

func (t *alterDiskTask) Run(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
) error {

	request := t.state.Request

	client, err := t.nbsFactory.GetClient(ctx, request.Disk.ZoneId)
	if err != nil {
		return err
	}

	return client.Alter(
		ctx,
		func() error {
			// Confirm that current generation is not obsolete (NBS-1292).
			return execCtx.SaveState(ctx)
		},
		request.Disk.DiskId,
		request.CloudId,
		request.FolderId,
	)
}

func (t *alterDiskTask) Cancel(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
) error {

	// TODO: should it be cancellable?
	return nil
}

func (t *alterDiskTask) GetMetadata(
	ctx context.Context,
) (proto.Message, error) {

	return &empty.Empty{}, nil
}

func (t *alterDiskTask) GetResponse() proto.Message {
	return &empty.Empty{}
}
