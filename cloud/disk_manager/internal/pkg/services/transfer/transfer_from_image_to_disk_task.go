package transfer

import (
	"context"
	"fmt"
	"time"

	"github.com/golang/protobuf/proto"
	"github.com/golang/protobuf/ptypes/empty"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/clients/snapshot"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/common"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/transfer/protos"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks"
)

////////////////////////////////////////////////////////////////////////////////

type transferFromImageToDiskTask struct {
	snapshotFactory          snapshot.Factory
	snapshotPingPeriod       time.Duration
	snapshotHeartbeatTimeout time.Duration
	snapshotBackoffTimeout   time.Duration
	state                    *protos.TransferFromImageToDiskTaskState
}

func (t *transferFromImageToDiskTask) Init(
	ctx context.Context,
	request proto.Message,
) error {

	typedRequest, ok := request.(*protos.TransferFromImageToDiskRequest)
	if !ok {
		return fmt.Errorf("invalid request=%v type", request)
	}

	state := &protos.TransferFromImageToDiskTaskState{
		Request: typedRequest,
	}
	t.state = state
	return nil
}

func (t *transferFromImageToDiskTask) Save(ctx context.Context) ([]byte, error) {
	return proto.Marshal(t.state)
}

func (t *transferFromImageToDiskTask) Load(ctx context.Context, state []byte) error {
	t.state = &protos.TransferFromImageToDiskTaskState{}
	return proto.Unmarshal(state, t.state)
}

func (t *transferFromImageToDiskTask) Run(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
) error {

	request := t.state.Request

	client, err := t.snapshotFactory.CreateClientFromZone(
		ctx,
		request.Dst.ZoneId,
	)
	if err != nil {
		return err
	}
	defer client.Close()

	err = client.TransferFromImageToDisk(
		ctx,
		snapshot.TransferFromImageToDiskState{
			SrcImageID:        request.SrcImageId,
			DstDisk:           request.Dst,
			OperationCloudID:  request.OperationCloudId,
			OperationFolderID: request.OperationFolderId,
			State: snapshot.TaskState{
				TaskID:           execCtx.GetTaskID(),
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
					execCtx.GetTaskID(),
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

	t.state.Progress = 1
	return nil
}

func (t *transferFromImageToDiskTask) Cancel(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
) error {

	// TODO: Implement me.
	return nil
}

func (t *transferFromImageToDiskTask) GetMetadata(
	ctx context.Context,
) (proto.Message, error) {

	return &protos.TransferFromImageToDiskMetadata{
		Progress: t.state.Progress,
	}, nil
}

func (t *transferFromImageToDiskTask) GetResponse() proto.Message {
	return &empty.Empty{}
}
