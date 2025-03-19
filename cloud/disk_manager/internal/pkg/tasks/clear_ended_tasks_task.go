package tasks

import (
	"context"
	"time"

	"github.com/golang/protobuf/proto"
	"github.com/golang/protobuf/ptypes/empty"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/storage"
)

////////////////////////////////////////////////////////////////////////////////

type clearEndedTasksTask struct {
	storage           storage.Storage
	expirationTimeout time.Duration
	limit             int
}

func (t *clearEndedTasksTask) Init(
	ctx context.Context,
	request proto.Message,
) error {

	return nil
}

func (t *clearEndedTasksTask) Save(ctx context.Context) ([]byte, error) {
	return proto.Marshal(&empty.Empty{})
}

func (t *clearEndedTasksTask) Load(ctx context.Context, state []byte) error {
	return nil
}

func (t *clearEndedTasksTask) Run(
	ctx context.Context,
	execCtx ExecutionContext,
) error {

	endedBefore := time.Now().Add(-t.expirationTimeout)
	return t.storage.ClearEndedTasks(ctx, endedBefore, t.limit)
}

func (t *clearEndedTasksTask) Cancel(
	ctx context.Context,
	execCtx ExecutionContext,
) error {

	return nil
}

func (t *clearEndedTasksTask) GetMetadata(
	ctx context.Context,
) (proto.Message, error) {

	return &empty.Empty{}, nil
}

func (t *clearEndedTasksTask) GetResponse() proto.Message {
	return &empty.Empty{}
}
