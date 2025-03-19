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

type deletePoolTask struct {
	storage storage.Storage
	state   *protos.DeletePoolTaskState
}

func (t *deletePoolTask) Init(
	ctx context.Context,
	request proto.Message,
) error {

	typedRequest, ok := request.(*protos.DeletePoolRequest)
	if !ok {
		return fmt.Errorf("invalid request=%v type", request)
	}

	state := &protos.DeletePoolTaskState{
		Request: typedRequest,
	}
	t.state = state
	return nil
}

func (t *deletePoolTask) Save(ctx context.Context) ([]byte, error) {
	return proto.Marshal(t.state)
}

func (t *deletePoolTask) Load(ctx context.Context, state []byte) error {
	t.state = &protos.DeletePoolTaskState{}
	return proto.Unmarshal(state, t.state)
}

func (t *deletePoolTask) deletePool(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
) error {

	return t.storage.DeletePool(
		ctx,
		t.state.Request.ImageId,
		t.state.Request.ZoneId,
	)
}

func (t *deletePoolTask) Run(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
) error {

	err := t.deletePool(ctx, execCtx)
	return errors.MakeRetriable(err, true /* ignoreRetryLimit */)
}

func (t *deletePoolTask) Cancel(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
) error {

	return t.deletePool(ctx, execCtx)
}

func (t *deletePoolTask) GetMetadata(
	ctx context.Context,
) (proto.Message, error) {

	return &empty.Empty{}, nil
}

func (t *deletePoolTask) GetResponse() proto.Message {
	return &empty.Empty{}
}
