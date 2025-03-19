package images

import (
	"context"
	"fmt"
	"time"

	"github.com/golang/protobuf/proto"
	"github.com/golang/protobuf/ptypes/empty"

	"a.yandex-team.ru/cloud/disk_manager/api/yandex/cloud/priv/disk_manager/v1"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/clients/snapshot"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/resources"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/images/protos"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/pools"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/errors"
)

////////////////////////////////////////////////////////////////////////////////

type deleteImageTask struct {
	scheduler                tasks.Scheduler
	storage                  resources.Storage
	snapshotFactory          snapshot.Factory
	snapshotPingPeriod       time.Duration
	snapshotHeartbeatTimeout time.Duration
	snapshotBackoffTimeout   time.Duration
	poolService              pools.Service
	state                    *protos.DeleteImageTaskState
}

func (t *deleteImageTask) Init(
	ctx context.Context,
	request proto.Message,
) error {

	typedRequest, ok := request.(*protos.DeleteImageRequest)
	if !ok {
		return fmt.Errorf("invalid request=%v type", request)
	}

	state := &protos.DeleteImageTaskState{
		Request: typedRequest,
	}
	t.state = state
	return nil
}

func (t *deleteImageTask) Save(ctx context.Context) ([]byte, error) {
	return proto.Marshal(t.state)
}

func (t *deleteImageTask) Load(ctx context.Context, state []byte) error {
	t.state = &protos.DeleteImageTaskState{}
	return proto.Unmarshal(state, t.state)
}

func (t *deleteImageTask) deleteImage(
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
		request.ImageId,
		request.OperationCloudId,
		request.OperationFolderId,
		t.snapshotHeartbeatTimeout,
		t.snapshotPingPeriod,
		t.snapshotBackoffTimeout,
	)
}

func (t *deleteImageTask) Run(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
) error {

	err := t.deleteImage(ctx, execCtx)
	return errors.MakeRetriable(err, true /* ignoreRetryLimit */)
}

func (t *deleteImageTask) Cancel(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
) error {

	return t.deleteImage(ctx, execCtx)
}

func (t *deleteImageTask) GetMetadata(
	ctx context.Context,
) (proto.Message, error) {

	return &disk_manager.DeleteImageMetadata{
		ImageId: t.state.Request.ImageId,
	}, nil
}

func (t *deleteImageTask) GetResponse() proto.Message {
	return &empty.Empty{}
}
