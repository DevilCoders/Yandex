package pools

import (
	"context"
	"fmt"

	"github.com/golang/protobuf/proto"
	"github.com/golang/protobuf/ptypes/empty"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/pools/protos"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/pools/storage"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/errors"
)

////////////////////////////////////////////////////////////////////////////////

type imageDeletingTask struct {
	storage storage.Storage
	state   *protos.ImageDeletingTaskState
}

func (t *imageDeletingTask) Init(
	ctx context.Context,
	request proto.Message,
) error {

	typedRequest, ok := request.(*protos.ImageDeletingRequest)
	if !ok {
		return fmt.Errorf("invalid request=%v type", request)
	}

	state := &protos.ImageDeletingTaskState{
		Request: typedRequest,
	}
	t.state = state
	return nil
}

func (t *imageDeletingTask) Save(ctx context.Context) ([]byte, error) {
	return proto.Marshal(t.state)
}

func (t *imageDeletingTask) Load(ctx context.Context, state []byte) error {
	t.state = &protos.ImageDeletingTaskState{}
	return proto.Unmarshal(state, t.state)
}

func (t *imageDeletingTask) imageDeleting(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
) error {

	return t.storage.ImageDeleting(ctx, t.state.Request.ImageId)
}

func (t *imageDeletingTask) Run(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
) error {

	err := t.imageDeleting(ctx, execCtx)
	return errors.MakeRetriable(err, true /* ignoreRetryLimit */)
}

func (t *imageDeletingTask) Cancel(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
) error {

	return t.imageDeleting(ctx, execCtx)
}

func (t *imageDeletingTask) GetMetadata(
	ctx context.Context,
) (proto.Message, error) {

	return &empty.Empty{}, nil
}

func (t *imageDeletingTask) GetResponse() proto.Message {
	return &empty.Empty{}
}
