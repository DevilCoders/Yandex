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

type resizeDiskTask struct {
	nbsFactory nbs.Factory
	state      *protos.ResizeDiskTaskState
}

func (t *resizeDiskTask) Init(
	ctx context.Context,
	request proto.Message,
) error {

	typedRequest, ok := request.(*protos.ResizeDiskRequest)
	if !ok {
		return fmt.Errorf("invalid request=%v type", request)
	}

	t.state = &protos.ResizeDiskTaskState{
		Request: typedRequest,
	}

	return nil
}

func (t *resizeDiskTask) Save(ctx context.Context) ([]byte, error) {
	return proto.Marshal(t.state)
}

func (t *resizeDiskTask) Load(ctx context.Context, state []byte) error {
	t.state = &protos.ResizeDiskTaskState{}
	return proto.Unmarshal(state, t.state)
}

func (t *resizeDiskTask) Run(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
) error {

	request := t.state.Request

	client, err := t.nbsFactory.GetClient(ctx, request.Disk.ZoneId)
	if err != nil {
		return err
	}

	return client.Resize(
		ctx,
		func() error {
			// Confirm that current generation is not obsolete (NBS-1292).
			return execCtx.SaveState(ctx)
		},
		request.Disk.DiskId,
		request.Size,
	)
}

func (t *resizeDiskTask) Cancel(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
) error {

	// TODO: should it be cancellable?
	return nil
}

func (t *resizeDiskTask) GetMetadata(
	ctx context.Context,
) (proto.Message, error) {

	return &empty.Empty{}, nil
}

func (t *resizeDiskTask) GetResponse() proto.Message {
	return &empty.Empty{}
}
