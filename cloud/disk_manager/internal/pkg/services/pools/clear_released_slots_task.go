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

type clearReleasedSlotsTask struct {
	storage           storage.Storage
	expirationTimeout time.Duration
	limit             int
}

func (t *clearReleasedSlotsTask) Init(
	ctx context.Context,
	request proto.Message,
) error {

	return nil
}

func (t *clearReleasedSlotsTask) Save(ctx context.Context) ([]byte, error) {
	return proto.Marshal(&empty.Empty{})
}

func (t *clearReleasedSlotsTask) Load(ctx context.Context, state []byte) error {
	return nil
}

func (t *clearReleasedSlotsTask) Run(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
) error {

	releasedBefore := time.Now().Add(-t.expirationTimeout)
	return t.storage.ClearReleasedSlots(ctx, releasedBefore, t.limit)
}

func (t *clearReleasedSlotsTask) Cancel(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
) error {

	return nil
}

func (t *clearReleasedSlotsTask) GetMetadata(
	ctx context.Context,
) (proto.Message, error) {

	return &empty.Empty{}, nil
}

func (t *clearReleasedSlotsTask) GetResponse() proto.Message {
	return &empty.Empty{}
}
