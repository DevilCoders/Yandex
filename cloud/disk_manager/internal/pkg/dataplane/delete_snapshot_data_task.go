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

type deleteSnapshotDataTask struct {
	storage storage.Storage
	state   *protos.DeleteSnapshotDataTaskState
}

func (t *deleteSnapshotDataTask) Init(
	ctx context.Context,
	request proto.Message,
) error {

	typedRequest, ok := request.(*protos.DeleteSnapshotDataRequest)
	if !ok {
		return fmt.Errorf("invalid request=%v type", request)
	}

	state := &protos.DeleteSnapshotDataTaskState{
		Request: typedRequest,
	}
	t.state = state
	return nil
}

func (t *deleteSnapshotDataTask) Save(ctx context.Context) ([]byte, error) {
	return proto.Marshal(t.state)
}

func (t *deleteSnapshotDataTask) Load(ctx context.Context, state []byte) error {
	t.state = &protos.DeleteSnapshotDataTaskState{}
	return proto.Unmarshal(state, t.state)
}

func (t *deleteSnapshotDataTask) deleteSnapshotData(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
) error {

	return t.storage.DeleteSnapshotData(ctx, t.state.Request.SnapshotId)
}

func (t *deleteSnapshotDataTask) Run(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
) error {

	err := t.deleteSnapshotData(ctx, execCtx)
	return errors.MakeRetriable(err, true /* ignoreRetryLimit */)
}

func (t *deleteSnapshotDataTask) Cancel(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
) error {

	return t.deleteSnapshotData(ctx, execCtx)
}

func (t *deleteSnapshotDataTask) GetMetadata(
	ctx context.Context,
) (proto.Message, error) {

	return &empty.Empty{}, nil
}

func (t *deleteSnapshotDataTask) GetResponse() proto.Message {
	return &empty.Empty{}
}
