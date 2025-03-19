package pools

import (
	"context"
	"time"

	"github.com/golang/protobuf/proto"
	"github.com/golang/protobuf/ptypes/empty"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/pools/storage"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks"
)

////////////////////////////////////////////////////////////////////////////////

type clearDeletedBaseDisksTask struct {
	storage           storage.Storage
	expirationTimeout time.Duration
	limit             int
}

func (t *clearDeletedBaseDisksTask) Init(
	ctx context.Context,
	request proto.Message,
) error {

	return nil
}

func (t *clearDeletedBaseDisksTask) Save(ctx context.Context) ([]byte, error) {
	return proto.Marshal(&empty.Empty{})
}

func (t *clearDeletedBaseDisksTask) Load(ctx context.Context, state []byte) error {
	return nil
}

func (t *clearDeletedBaseDisksTask) Run(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
) error {

	deletedBefore := time.Now().Add(-t.expirationTimeout)
	return t.storage.ClearDeletedBaseDisks(ctx, deletedBefore, t.limit)
}

func (t *clearDeletedBaseDisksTask) Cancel(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
) error {

	return nil
}

func (t *clearDeletedBaseDisksTask) GetMetadata(
	ctx context.Context,
) (proto.Message, error) {

	return &empty.Empty{}, nil
}

func (t *clearDeletedBaseDisksTask) GetResponse() proto.Message {
	return &empty.Empty{}
}
