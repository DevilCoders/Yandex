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
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/performance"
	performance_config "a.yandex-team.ru/cloud/disk_manager/internal/pkg/performance/config"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/resources"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/images/protos"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/pools"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/errors"
)

////////////////////////////////////////////////////////////////////////////////

type createImageFromSnapshotTask struct {
	performanceConfig        *performance_config.PerformanceConfig
	scheduler                tasks.Scheduler
	storage                  resources.Storage
	snapshotFactory          snapshot.Factory
	snapshotPingPeriod       time.Duration
	snapshotHeartbeatTimeout time.Duration
	snapshotBackoffTimeout   time.Duration
	poolService              pools.Service
	state                    *protos.CreateImageFromSnapshotTaskState
}

func (t *createImageFromSnapshotTask) Init(
	ctx context.Context,
	request proto.Message,
) error {

	typedRequest, ok := request.(*protos.CreateImageFromSnapshotRequest)
	if !ok {
		return fmt.Errorf("invalid request=%v type", request)
	}

	state := &protos.CreateImageFromSnapshotTaskState{
		Request: typedRequest,
	}
	t.state = state
	return nil
}

func (t *createImageFromSnapshotTask) Save(ctx context.Context) ([]byte, error) {
	return proto.Marshal(t.state)
}

func (t *createImageFromSnapshotTask) Load(ctx context.Context, state []byte) error {
	t.state = &protos.CreateImageFromSnapshotTaskState{}
	return proto.Unmarshal(state, t.state)
}

func (t *createImageFromSnapshotTask) Run(
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

	s, err := t.storage.GetSnapshotMeta(ctx, request.SrcSnapshotId)
	if err != nil {
		return err
	}

	srcIsDataplane := s != nil && s.UseDataplaneTasks

	imageMeta, err := t.storage.CreateImage(ctx, resources.ImageMeta{
		ID:                request.DstImageId,
		FolderID:          request.FolderId,
		CreateRequest:     request,
		CreateTaskID:      selfTaskID,
		CreatingAt:        time.Now(),
		CreatedBy:         "", // TODO: Extract CreatedBy from execCtx
		UseDataplaneTasks: srcIsDataplane || request.UseDataplaneTasksForLegacySnapshots,
	})
	if err != nil {
		return err
	}

	if imageMeta == nil {
		return &errors.NonCancellableError{
			Err: fmt.Errorf("id=%v is not accepted", request.DstImageId),
		}
	}

	if srcIsDataplane {
		taskID, err := t.scheduler.ScheduleTask(
			headers.SetIncomingIdempotencyKey(ctx, selfTaskID+"_run"),
			"dataplane.CreateSnapshotFromSnapshot",
			"",
			&dataplane_protos.CreateSnapshotFromSnapshotRequest{
				SrcSnapshotId: request.SrcSnapshotId,
				DstSnapshotId: request.DstImageId,
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

		typedResponse, ok := response.(*dataplane_protos.CreateSnapshotFromSnapshotResponse)
		if !ok {
			return fmt.Errorf("invalid dataplane.CreateSnapshotFromSnapshot response type")
		}

		// TODO: estimate should be applied before resource creation, not after.
		execCtx.SetEstimate(performance.Estimate(
			typedResponse.SnapshotStorageSize,
			t.performanceConfig.GetCreateImageFromSnapshotBandwidthMiBs(),
		))

		t.state.ImageSize = int64(typedResponse.SnapshotSize)
		t.state.ImageStorageSize = int64(typedResponse.SnapshotStorageSize)
	} else if imageMeta.UseDataplaneTasks {
		taskID, err := t.scheduler.ScheduleTask(
			headers.SetIncomingIdempotencyKey(ctx, selfTaskID+"_run"),
			"dataplane.CreateSnapshotFromLegacySnapshot",
			"",
			&dataplane_protos.CreateSnapshotFromLegacySnapshotRequest{
				SrcSnapshotId: request.SrcSnapshotId,
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

		typedResponse, ok := response.(*dataplane_protos.CreateSnapshotFromLegacySnapshotResponse)
		if !ok {
			return fmt.Errorf("invalid dataplane.CreateSnapshotFromLegacySnapshot response type")
		}

		t.state.ImageSize = int64(typedResponse.SnapshotSize)
		t.state.ImageStorageSize = int64(typedResponse.SnapshotStorageSize)
	} else {
		info, err := snapshotClient.CreateImageFromSnapshot(
			ctx,
			snapshot.CreateImageFromSnapshotState{
				SrcSnapshotID:     request.SrcSnapshotId,
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

func (t *createImageFromSnapshotTask) Cancel(
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

func (t *createImageFromSnapshotTask) GetMetadata(
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

		createMetadata, ok := message.(*dataplane_protos.CreateSnapshotFromSnapshotMetadata)
		if ok {
			metadata.Progress = createMetadata.Progress
		} else {
			createMetadata, ok := message.(*dataplane_protos.CreateSnapshotFromLegacySnapshotMetadata)
			if ok {
				metadata.Progress = createMetadata.Progress
			}
		}
	} else {
		metadata.Progress = t.state.Progress
	}

	return metadata, nil
}

func (t *createImageFromSnapshotTask) GetResponse() proto.Message {
	return &disk_manager.CreateImageResponse{
		Size:        t.state.ImageSize,
		StorageSize: t.state.ImageStorageSize,
	}
}
