package filesystem

import (
	"context"
	"fmt"
	"time"

	"github.com/golang/protobuf/proto"
	"github.com/golang/protobuf/ptypes/empty"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/clients/nfs"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/resources"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/filesystem/protos"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/errors"
)

////////////////////////////////////////////////////////////////////////////////

type createFilesystemTask struct {
	storage resources.Storage
	factory nfs.Factory
	state   *protos.CreateFilesystemTaskState
}

func (t *createFilesystemTask) Init(
	ctx context.Context,
	request proto.Message,
) error {

	typedRequest, ok := request.(*protos.CreateFilesystemRequest)
	if !ok {
		return fmt.Errorf("invalid request=%v type", request)
	}

	t.state = &protos.CreateFilesystemTaskState{
		Request: typedRequest,
	}

	return nil
}

func (t *createFilesystemTask) Save(ctx context.Context) ([]byte, error) {
	return proto.Marshal(t.state)
}

func (t *createFilesystemTask) Load(ctx context.Context, state []byte) error {
	t.state = &protos.CreateFilesystemTaskState{}
	return proto.Unmarshal(state, t.state)
}

func (t *createFilesystemTask) Run(
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

	filesystemMeta, err := t.storage.CreateFilesystem(ctx, resources.FilesystemMeta{
		ID:          request.Filesystem.FilesystemId,
		ZoneID:      request.Filesystem.ZoneId,
		BlocksCount: request.BlocksCount,
		BlockSize:   request.BlockSize,
		Kind:        fsKindToString(request.StorageKind),
		CloudID:     request.CloudId,
		FolderID:    request.FolderId,

		CreateRequest: request,
		CreateTaskID:  selfTaskID,
		CreatingAt:    time.Now(),
		CreatedBy:     "", // TODO: Extract CreatedBy from execCtx
	})
	if err != nil {
		return err
	}

	if filesystemMeta == nil {
		return &errors.NonCancellableError{
			Err: fmt.Errorf("id=%v is not accepted", request.Filesystem.FilesystemId),
		}
	}

	err = client.Create(ctx, request.Filesystem.FilesystemId, nfs.CreateFilesystemParams{
		CloudID:     request.CloudId,
		FolderID:    request.FolderId,
		BlocksCount: request.BlocksCount,
		BlockSize:   request.BlockSize,
		StorageKind: request.StorageKind,
	})
	if err != nil {
		return err
	}

	filesystemMeta.CreatedAt = time.Now()
	return t.storage.FilesystemCreated(ctx, *filesystemMeta)
}

func (t *createFilesystemTask) Cancel(
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

	fs, err := t.storage.DeleteFilesystem(
		ctx,
		request.Filesystem.FilesystemId,
		selfTaskID,
		time.Now(),
	)
	if err != nil {
		return err
	}

	if fs == nil {
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

func (t *createFilesystemTask) GetMetadata(
	ctx context.Context,
) (proto.Message, error) {

	return &empty.Empty{}, nil
}

func (t *createFilesystemTask) GetResponse() proto.Message {
	return &empty.Empty{}
}
