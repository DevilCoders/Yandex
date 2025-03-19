package disks

import (
	"context"
	"fmt"
	"time"

	"github.com/golang/protobuf/proto"
	"github.com/golang/protobuf/ptypes/empty"

	"a.yandex-team.ru/cloud/disk_manager/api/yandex/cloud/priv/disk_manager/v1"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/clients/nbs"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/resources"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/disks/protos"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/errors"
)

////////////////////////////////////////////////////////////////////////////////

type createEmptyDiskTask struct {
	storage    resources.Storage
	scheduler  tasks.Scheduler
	nbsFactory nbs.Factory
	state      *protos.CreateEmptyDiskTaskState
}

func (t *createEmptyDiskTask) Init(
	ctx context.Context,
	request proto.Message,
) error {

	params, ok := request.(*protos.CreateDiskParams)
	if !ok {
		return fmt.Errorf("invalid request=%v type", request)
	}

	t.state = &protos.CreateEmptyDiskTaskState{
		Params: params,
	}

	return nil
}

func (t *createEmptyDiskTask) Save(ctx context.Context) ([]byte, error) {
	return proto.Marshal(t.state)
}

func (t *createEmptyDiskTask) Load(ctx context.Context, state []byte) error {
	t.state = &protos.CreateEmptyDiskTaskState{}
	return proto.Unmarshal(state, t.state)
}

func (t *createEmptyDiskTask) Run(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
) error {

	params := t.state.Params

	client, err := t.nbsFactory.GetClient(ctx, params.Disk.ZoneId)
	if err != nil {
		return err
	}

	selfTaskID := execCtx.GetTaskID()

	diskMeta, err := t.storage.CreateDisk(ctx, resources.DiskMeta{
		ID:          params.Disk.DiskId,
		ZoneID:      params.Disk.ZoneId,
		BlocksCount: params.BlocksCount,
		BlockSize:   params.BlockSize,
		Kind:        diskKindToString(params.Kind),
		CloudID:     params.CloudId,
		FolderID:    params.FolderId,

		CreateRequest: params,
		CreateTaskID:  selfTaskID,
		CreatingAt:    time.Now(),
		CreatedBy:     "", // TODO: Extract CreatedBy from execCtx
	})
	if err != nil {
		return err
	}

	if diskMeta == nil {
		return &errors.NonCancellableError{
			Err: fmt.Errorf("id=%v is not accepted", params.Disk.DiskId),
		}
	}

	err = client.Create(ctx, nbs.CreateDiskParams{
		ID:               params.Disk.DiskId,
		BlocksCount:      params.BlocksCount,
		BlockSize:        params.BlockSize,
		Kind:             params.Kind,
		CloudID:          params.CloudId,
		FolderID:         params.FolderId,
		TabletVersion:    params.TabletVersion,
		PlacementGroupID: params.PlacementGroupId,
		StoragePoolName:  params.StoragePoolName,
		AgentIds:         params.AgentIds,
	})
	if err != nil {
		return err
	}

	diskMeta.CreatedAt = time.Now()
	return t.storage.DiskCreated(ctx, *diskMeta)
}

func (t *createEmptyDiskTask) Cancel(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
) error {

	params := t.state.Params

	client, err := t.nbsFactory.GetClient(ctx, params.Disk.ZoneId)
	if err != nil {
		return err
	}

	selfTaskID := execCtx.GetTaskID()

	diskMeta, err := t.storage.DeleteDisk(
		ctx,
		params.Disk.DiskId,
		selfTaskID,
		time.Now(),
	)
	if err != nil {
		return err
	}

	if diskMeta == nil {
		return &errors.NonCancellableError{
			Err: fmt.Errorf("id=%v is not accepted", params.Disk.DiskId),
		}
	}

	err = client.Delete(ctx, params.Disk.DiskId)
	if err != nil {
		return err
	}

	return t.storage.DiskDeleted(ctx, params.Disk.DiskId, time.Now())
}

func (t *createEmptyDiskTask) GetMetadata(
	ctx context.Context,
) (proto.Message, error) {

	return &disk_manager.CreateDiskMetadata{}, nil
}

func (t *createEmptyDiskTask) GetResponse() proto.Message {
	return &empty.Empty{}
}
