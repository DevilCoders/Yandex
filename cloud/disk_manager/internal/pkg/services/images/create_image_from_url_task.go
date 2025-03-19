package images

import (
	"context"
	"fmt"
	"time"

	"github.com/golang/protobuf/proto"

	"a.yandex-team.ru/cloud/disk_manager/api/yandex/cloud/priv/disk_manager/v1"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/clients/snapshot"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/common"
	dataplane_protos "a.yandex-team.ru/cloud/disk_manager/internal/pkg/dataplane/protos"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/headers"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/resources"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/images/protos"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/pools"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/errors"
	url_package "a.yandex-team.ru/cloud/disk_manager/internal/pkg/url"
)

////////////////////////////////////////////////////////////////////////////////

type createImageFromURLTask struct {
	scheduler                tasks.Scheduler
	storage                  resources.Storage
	snapshotFactory          snapshot.Factory
	snapshotPingPeriod       time.Duration
	snapshotHeartbeatTimeout time.Duration
	snapshotBackoffTimeout   time.Duration
	poolService              pools.Service
	state                    *protos.CreateImageFromURLTaskState
}

func (t *createImageFromURLTask) Init(
	ctx context.Context,
	request proto.Message,
) error {

	typedRequest, ok := request.(*protos.CreateImageFromURLRequest)
	if !ok {
		return fmt.Errorf("invalid request=%v type", request)
	}

	state := &protos.CreateImageFromURLTaskState{
		Request: typedRequest,
	}
	t.state = state
	return nil
}

func (t *createImageFromURLTask) Save(ctx context.Context) ([]byte, error) {
	return proto.Marshal(t.state)
}

func (t *createImageFromURLTask) Load(ctx context.Context, state []byte) error {
	t.state = &protos.CreateImageFromURLTaskState{}
	return proto.Unmarshal(state, t.state)
}

func (t *createImageFromURLTask) Run(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
) error {

	snapshotClient, err := t.snapshotFactory.CreateClient(ctx)
	if err != nil {
		return err
	}
	defer snapshotClient.Close()

	request := t.state.Request
	selfTaskID := execCtx.GetTaskID()

	// Temporary fix. Later only dataplane will be used, see NBS-3291.
	useDataplaneTasks := request.UseDataplaneTasks
	if useDataplaneTasks && request.UseDataplaneTasksQCOW2Only {
		isQCOW2URL, err := t.isQCOW2URL(ctx, execCtx)
		if err != nil {
			return err
		}

		if !isQCOW2URL {
			useDataplaneTasks = false
		}
	}

	imageMeta, err := t.storage.CreateImage(ctx, resources.ImageMeta{
		ID:                request.DstImageId,
		FolderID:          request.FolderId,
		CreateRequest:     request,
		CreateTaskID:      selfTaskID,
		CreatingAt:        time.Now(),
		CreatedBy:         "", // TODO: Extract CreatedBy from execCtx
		UseDataplaneTasks: useDataplaneTasks,
	})
	if err != nil {
		return err
	}

	if imageMeta == nil {
		return &errors.NonCancellableError{
			Err: fmt.Errorf("id=%v is not accepted", request.DstImageId),
		}
	}

	if imageMeta.UseDataplaneTasks {
		taskID, err := t.scheduler.ScheduleTask(
			headers.SetIncomingIdempotencyKey(ctx, selfTaskID+"_run"),
			"dataplane.CreateSnapshotFromURL",
			"",
			&dataplane_protos.CreateSnapshotFromURLRequest{
				SrcURL:        request.SrcURL,
				DstSnapshotId: request.DstImageId,
				UseS3:         request.UseS3,
			},
			request.OperationCloudId,
			request.OperationFolderId,
		)
		if err != nil {
			return err
		}

		t.state.DataplaneTaskID = taskID

		response, err := t.scheduler.WaitTaskAsync(ctx, execCtx, taskID)
		if err != nil {
			return err
		}

		typedResponse, ok := response.(*dataplane_protos.CreateSnapshotFromURLResponse)
		if !ok {
			return fmt.Errorf("invalid dataplane.CreateSnapshotFromURL response type: %T", response)
		}

		t.state.ImageSize = int64(typedResponse.SnapshotSize)
		t.state.ImageStorageSize = int64(typedResponse.SnapshotStorageSize)
	} else {
		info, err := snapshotClient.CreateImageFromURL(
			ctx,
			snapshot.CreateImageFromURLState{
				SrcURL:            request.SrcURL,
				DstImageID:        request.DstImageId,
				FolderID:          request.FolderId,
				OperationCloudID:  request.OperationCloudId,
				OperationFolderID: request.OperationFolderId,
				State: snapshot.TaskState{
					TaskID:           selfTaskID + "_run",
					Offset:           t.state.Offset,
					HeartbeatTimeout: t.snapshotHeartbeatTimeout,
					PingPeriod:       t.snapshotPingPeriod,
					BackoffTimeout:   t.snapshotBackoffTimeout,
				},
			},
			func(offset int64, progress float64) error {
				common.Assert(
					t.state.Offset <= offset,
					fmt.Sprintf(
						"offset should not run backward: %v > %v, taskID=%v",
						offset,
						t.state.Offset,
						selfTaskID,
					),
				)

				t.state.Offset = offset
				t.state.Progress = progress
				return execCtx.SaveState(ctx)
			},
		)
		if err != nil {
			return err
		}

		t.state.ImageSize = info.Size
		t.state.ImageStorageSize = info.StorageSize
		t.state.Progress = 1
	}

	err = t.storage.ImageCreated(
		ctx,
		request.DstImageId,
		time.Now(),
		uint64(t.state.ImageSize),
		uint64(t.state.ImageStorageSize),
	)
	if err != nil {
		return err
	}

	return configureImagePools(
		ctx,
		execCtx,
		t.scheduler,
		t.poolService,
		request.DstImageId,
		request.DiskPools,
	)
}

func (t *createImageFromURLTask) Cancel(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
) error {

	snapshotClient, err := t.snapshotFactory.CreateClient(ctx)
	if err != nil {
		return err
	}
	defer snapshotClient.Close()

	request := t.state.Request

	return deleteImage(
		ctx,
		execCtx,
		t.scheduler,
		t.storage,
		snapshotClient,
		t.poolService,
		request.DstImageId,
		request.OperationCloudId,
		request.OperationFolderId,
		t.snapshotHeartbeatTimeout,
		t.snapshotPingPeriod,
		t.snapshotBackoffTimeout,
	)
}

func (t *createImageFromURLTask) GetMetadata(
	ctx context.Context,
) (proto.Message, error) {

	metadata := &disk_manager.CreateImageMetadata{}

	if len(t.state.DataplaneTaskID) != 0 {
		message, err := t.scheduler.GetTaskMetadata(
			ctx,
			t.state.DataplaneTaskID,
		)
		if err != nil {
			return nil, err
		}

		createMetadata, ok := message.(*dataplane_protos.CreateSnapshotFromURLMetadata)
		if ok {
			metadata.Progress = createMetadata.Progress
		}
	} else {
		metadata.Progress = t.state.Progress
	}

	return metadata, nil
}

func (t *createImageFromURLTask) GetResponse() proto.Message {
	return &disk_manager.CreateImageResponse{
		Size:        t.state.ImageSize,
		StorageSize: t.state.ImageStorageSize,
	}
}

////////////////////////////////////////////////////////////////////////////////

func (t *createImageFromURLTask) isQCOW2URL(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
) (bool, error) {

	if t.state.IsQCOW2URL == nil {
		reader, err := url_package.NewImageMapReader(
			ctx,
			t.state.Request.SrcURL,
		)
		if err != nil {
			return false, err
		}

		isQCOW2URL := reader.ReadingQCOW2()
		t.state.IsQCOW2URL = &isQCOW2URL

		err = execCtx.SaveState(ctx)
		if err != nil {
			return false, err
		}
	}

	return t.state.GetIsQCOW2URL(), nil
}
