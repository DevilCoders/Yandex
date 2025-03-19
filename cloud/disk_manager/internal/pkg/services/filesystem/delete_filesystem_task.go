package filesystem

import (
	"context"
	"fmt"
	"time"

	"github.com/golang/protobuf/proto"
	"github.com/golang/protobuf/ptypes/empty"

	"a.yandex-team.ru/cloud/disk_manager/api/yandex/cloud/priv/disk_manager/v1"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/clients/nfs"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/resources"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/filesystem/protos"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/errors"
)

////////////////////////////////////////////////////////////////////////////////

type deleteFilesystemTask struct {
	storage resources.Storage
	factory nfs.Factory
	state   *protos.DeleteFilesystemTaskState
}

func (t *deleteFilesystemTask) Init(
	ctx context.Context,
	request proto.Message,
) error {

	typedRequest, ok := request.(*protos.DeleteFilesystemRequest)
	if !ok {
		return fmt.Errorf("invalid request=%v type", request)
	}

	t.state = &protos.DeleteFilesystemTaskState{
		Request: typedRequest,
	}

	return nil
}

func (t *deleteFilesystemTask) Save(ctx context.Context) ([]byte, error) {
	return proto.Marshal(t.state)
}

func (t *deleteFilesystemTask) Load(ctx context.Context, state []byte) error {
	t.state = &protos.DeleteFilesystemTaskState{}
	return proto.Unmarshal(state, t.state)
}

func (t *deleteFilesystemTask) deleteFilesystem(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
) error {

	request := t.state.Request

	client, err := t.factory.CreateClient(ctx, request.Filesystem.ZoneId)
	if err != nil {
		return err
	}
	defer client.Close()

	selfTaskID := execCtx.GetTaskID()

	filesystemMeta, err := t.storage.DeleteFilesystem(
		ctx,
		request.Filesystem.FilesystemId,
		selfTaskID,
		time.Now(),
	)
	if err != nil {
		return err
	}

	if filesystemMeta == nil {
		return &errors.NonCancellableError{
			Err: fmt.Errorf("id=%v is not accepted", request.Filesystem.FilesystemId),
		}
	}

	err = client.Delete(ctx, request.Filesystem.FilesystemId)
	if err != nil {
		return err
	}

	return t.storage.FilesystemDeleted(ctx, request.Filesystem.FilesystemId, time.Now())
}

func (t *deleteFilesystemTask) Run(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
) error {

	err := t.deleteFilesystem(ctx, execCtx)
	return errors.MakeRetriable(err, true /* ignoreRetryLimit */)
}

func (t *deleteFilesystemTask) Cancel(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
) error {

	return t.deleteFilesystem(ctx, execCtx)
}

func (t *deleteFilesystemTask) GetMetadata(
	ctx context.Context,
) (proto.Message, error) {

	return &disk_manager.DeleteFilesystemMetadata{
		FilesystemId: &disk_manager.FilesystemId{
			ZoneId:       t.state.Request.Filesystem.ZoneId,
			FilesystemId: t.state.Request.Filesystem.FilesystemId,
		},
	}, nil
}

func (t *deleteFilesystemTask) GetResponse() proto.Message {
	return &empty.Empty{}
}
