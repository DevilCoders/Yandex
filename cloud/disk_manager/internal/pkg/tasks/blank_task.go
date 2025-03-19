package tasks

import (
	"context"

	"github.com/golang/protobuf/proto"
	"github.com/golang/protobuf/ptypes/empty"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/logging"
)

////////////////////////////////////////////////////////////////////////////////

type blankTask struct {
}

func (t *blankTask) Init(
	ctx context.Context,
	request proto.Message,
) error {

	return nil
}

func (t *blankTask) Save(ctx context.Context) ([]byte, error) {
	return proto.Marshal(&empty.Empty{})
}

func (t *blankTask) Load(ctx context.Context, state []byte) error {
	return nil
}

func (t *blankTask) Run(
	ctx context.Context,
	execCtx ExecutionContext,
) error {

	logging.Info(ctx, "tasks.Blank running with id=%v", execCtx.GetTaskID())
	return nil
}

func (t *blankTask) Cancel(
	ctx context.Context,
	execCtx ExecutionContext,
) error {

	logging.Info(ctx, "tasks.Blank cancelling with id=%v", execCtx.GetTaskID())
	return nil
}

func (t *blankTask) GetMetadata(
	ctx context.Context,
) (proto.Message, error) {

	return &empty.Empty{}, nil
}

func (t *blankTask) GetResponse() proto.Message {
	return &empty.Empty{}
}
