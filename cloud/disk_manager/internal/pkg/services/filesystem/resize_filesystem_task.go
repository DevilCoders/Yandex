package filesystem

import (
	"context"
	"fmt"

	"github.com/golang/protobuf/proto"
	"github.com/golang/protobuf/ptypes/empty"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/clients/nfs"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/filesystem/protos"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks"
)

////////////////////////////////////////////////////////////////////////////////

type resizeFilesystemTask struct {
	factory nfs.Factory
	state   *protos.ResizeFilesystemTaskState
}

func (t *resizeFilesystemTask) Init(
	ctx context.Context,
	request proto.Message,
) error {

	typedRequest, ok := request.(*protos.ResizeFilesystemRequest)
	if !ok {
		return fmt.Errorf("invalid request=%v type", request)
	}

	t.state = &protos.ResizeFilesystemTaskState{
		Request: typedRequest,
	}

	return nil
}

func (t *resizeFilesystemTask) Save(ctx context.Context) ([]byte, error) {
	return proto.Marshal(t.state)
}

func (t *resizeFilesystemTask) Load(ctx context.Context, state []byte) error {
	t.state = &protos.ResizeFilesystemTaskState{}
	return proto.Unmarshal(state, t.state)
}

func (t *resizeFilesystemTask) Run(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
) error {

	request := t.state.Request

	client, err := t.factory.CreateClient(ctx, request.Filesystem.ZoneId)
	if err != nil {
		return err
	}
	defer client.Close()

	return client.Resize(ctx, request.Filesystem.FilesystemId, request.Size)
}

func (t *resizeFilesystemTask) Cancel(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
) error {

	return nil
}

func (t *resizeFilesystemTask) GetMetadata(
	ctx context.Context,
) (proto.Message, error) {

	return &empty.Empty{}, nil
}

func (t *resizeFilesystemTask) GetResponse() proto.Message {
	return &empty.Empty{}
}
