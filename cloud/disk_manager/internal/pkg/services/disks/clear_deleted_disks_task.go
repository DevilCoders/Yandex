package disks

import (
	"context"
	"time"

	"github.com/golang/protobuf/proto"
	"github.com/golang/protobuf/ptypes/empty"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/resources"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks"
)

////////////////////////////////////////////////////////////////////////////////

type clearDeletedDisksTask struct {
	storage           resources.Storage
	expirationTimeout time.Duration
	limit             int
}

func (t *clearDeletedDisksTask) Init(
	ctx context.Context,
	request proto.Message,
) error {

	return nil
}

func (t *clearDeletedDisksTask) Save(ctx context.Context) ([]byte, error) {
	return proto.Marshal(&empty.Empty{})
}

func (t *clearDeletedDisksTask) Load(ctx context.Context, state []byte) error {
	return nil
}

func (t *clearDeletedDisksTask) Run(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
) error {

	deletedBefore := time.Now().Add(-t.expirationTimeout)
	return t.storage.ClearDeletedDisks(ctx, deletedBefore, t.limit)
}

func (t *clearDeletedDisksTask) Cancel(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
) error {

	return nil
}

func (t *clearDeletedDisksTask) GetMetadata(
	ctx context.Context,
) (proto.Message, error) {

	return &empty.Empty{}, nil
}

func (t *clearDeletedDisksTask) GetResponse() proto.Message {
	return &empty.Empty{}
}
