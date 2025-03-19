package snapshots

import (
	"context"
	"time"

	"github.com/golang/protobuf/proto"
	"github.com/golang/protobuf/ptypes/empty"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/resources"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks"
)

////////////////////////////////////////////////////////////////////////////////

type clearDeletedSnapshotsTask struct {
	storage           resources.Storage
	expirationTimeout time.Duration
	limit             int
}

func (t *clearDeletedSnapshotsTask) Init(
	ctx context.Context,
	request proto.Message,
) error {

	return nil
}

func (t *clearDeletedSnapshotsTask) Save(ctx context.Context) ([]byte, error) {
	return proto.Marshal(&empty.Empty{})
}

func (t *clearDeletedSnapshotsTask) Load(ctx context.Context, state []byte) error {
	return nil
}

func (t *clearDeletedSnapshotsTask) Run(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
) error {

	deletedBefore := time.Now().Add(-t.expirationTimeout)
	return t.storage.ClearDeletedSnapshots(ctx, deletedBefore, t.limit)
}

func (t *clearDeletedSnapshotsTask) Cancel(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
) error {

	return nil
}

func (t *clearDeletedSnapshotsTask) GetMetadata(
	ctx context.Context,
) (proto.Message, error) {

	return &empty.Empty{}, nil
}

func (t *clearDeletedSnapshotsTask) GetResponse() proto.Message {
	return &empty.Empty{}
}
