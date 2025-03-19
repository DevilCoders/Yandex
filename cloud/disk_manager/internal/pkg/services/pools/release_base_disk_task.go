package pools

import (
	"context"
	"fmt"

	"github.com/golang/protobuf/proto"
	"github.com/golang/protobuf/ptypes/empty"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/logging"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/pools/protos"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/pools/storage"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/errors"
)

////////////////////////////////////////////////////////////////////////////////

type releaseBaseDiskTask struct {
	storage storage.Storage
	state   *protos.ReleaseBaseDiskTaskState
}

func (t *releaseBaseDiskTask) Init(
	ctx context.Context,
	request proto.Message,
) error {

	typedRequest, ok := request.(*protos.ReleaseBaseDiskRequest)
	if !ok {
		return fmt.Errorf("invalid request=%v type", request)
	}

	state := &protos.ReleaseBaseDiskTaskState{
		Request: typedRequest,
	}
	t.state = state
	return nil
}

func (t *releaseBaseDiskTask) Save(ctx context.Context) ([]byte, error) {
	return proto.Marshal(t.state)
}

func (t *releaseBaseDiskTask) Load(ctx context.Context, state []byte) error {
	t.state = &protos.ReleaseBaseDiskTaskState{}
	return proto.Unmarshal(state, t.state)
}

func (t *releaseBaseDiskTask) releaseBaseDisk(ctx context.Context) error {
	overlayDisk := t.state.Request.OverlayDisk

	baseDisk, err := t.storage.ReleaseBaseDiskSlot(ctx, overlayDisk)
	if err == nil {
		logging.Debug(
			ctx,
			"releaseBaseDiskTask: overlayDisk=%v released slot on baseDisk=%v",
			overlayDisk,
			baseDisk,
		)
	}

	return err
}

func (t *releaseBaseDiskTask) Run(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
) error {

	err := t.releaseBaseDisk(ctx)
	return errors.MakeRetriable(err, true /* ignoreRetryLimit */)
}

func (t *releaseBaseDiskTask) Cancel(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
) error {

	return t.releaseBaseDisk(ctx)
}

func (t *releaseBaseDiskTask) GetMetadata(
	ctx context.Context,
) (proto.Message, error) {

	return &empty.Empty{}, nil
}

func (t *releaseBaseDiskTask) GetResponse() proto.Message {
	return &empty.Empty{}
}
