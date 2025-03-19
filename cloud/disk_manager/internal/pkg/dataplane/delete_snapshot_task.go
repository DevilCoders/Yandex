package dataplane

import (
	"context"
	"fmt"

	"github.com/golang/protobuf/proto"
	"github.com/golang/protobuf/ptypes/empty"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/dataplane/protos"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/dataplane/snapshot/storage"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/errors"
)

////////////////////////////////////////////////////////////////////////////////

type deleteSnapshotTask struct {
	storage storage.Storage
	state   *protos.DeleteSnapshotTaskState
}

func (t *deleteSnapshotTask) Init(
	ctx context.Context,
	request proto.Message,
) error {

	typedRequest, ok := request.(*protos.DeleteSnapshotRequest)
	if !ok {
		return fmt.Errorf("invalid request=%v type", request)
	}

	state := &protos.DeleteSnapshotTaskState{
		Request: typedRequest,
	}
	t.state = state
	return nil
}

func (t *deleteSnapshotTask) Save(ctx context.Context) ([]byte, error) {
	return proto.Marshal(t.state)
}

func (t *deleteSnapshotTask) Load(ctx context.Context, state []byte) error {
	t.state = &protos.DeleteSnapshotTaskState{}
	return proto.Unmarshal(state, t.state)
}

func (t *deleteSnapshotTask) deletingSnapshot(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
) error {

	return t.storage.DeletingSnapshot(ctx, t.state.Request.SnapshotId)
}

func (t *deleteSnapshotTask) Run(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
) error {

	err := t.deletingSnapshot(ctx, execCtx)
	return errors.MakeRetriable(err, true /* ignoreRetryLimit */)
}

func (t *deleteSnapshotTask) Cancel(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
) error {

	return t.deletingSnapshot(ctx, execCtx)
}

func (t *deleteSnapshotTask) GetMetadata(
	ctx context.Context,
) (proto.Message, error) {

	return &empty.Empty{}, nil
}

func (t *deleteSnapshotTask) GetResponse() proto.Message {
	return &empty.Empty{}
}
