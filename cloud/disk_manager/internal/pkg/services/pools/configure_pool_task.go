package pools

import (
	"context"
	"fmt"

	"github.com/golang/protobuf/proto"
	"github.com/golang/protobuf/ptypes/empty"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/clients/snapshot"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/resources"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/pools/protos"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/pools/storage"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks"
)

////////////////////////////////////////////////////////////////////////////////

type configurePoolTask struct {
	storage         storage.Storage
	snapshotFactory snapshot.Factory
	resourceStorage resources.Storage
	state           *protos.ConfigurePoolTaskState
}

func (t *configurePoolTask) Init(
	ctx context.Context,
	request proto.Message,
) error {

	typedRequest, ok := request.(*protos.ConfigurePoolRequest)
	if !ok {
		return fmt.Errorf("invalid request=%v type", request)
	}

	state := &protos.ConfigurePoolTaskState{
		Request: typedRequest,
	}
	t.state = state
	return nil
}

func (t *configurePoolTask) Save(ctx context.Context) ([]byte, error) {
	return proto.Marshal(t.state)
}

func (t *configurePoolTask) Load(ctx context.Context, state []byte) error {
	t.state = &protos.ConfigurePoolTaskState{}
	return proto.Unmarshal(state, t.state)
}

func (t *configurePoolTask) checkImage(
	ctx context.Context,
) (uint64, error) {

	request := t.state.Request

	image, err := t.resourceStorage.GetImageMeta(ctx, request.ImageId)
	if err != nil {
		return 0, err
	}

	if image != nil && image.UseDataplaneTasks {
		_, err = t.resourceStorage.CheckImageReady(ctx, request.ImageId)
		if err != nil {
			return 0, err
		}

		return image.Size, nil
	} else {
		snapshotClient, err := t.snapshotFactory.CreateClientFromZone(
			ctx,
			request.ZoneId,
		)
		if err != nil {
			return 0, err
		}
		defer snapshotClient.Close()

		resourceInfo, err := snapshotClient.CheckResourceReady(ctx, request.ImageId)
		if err != nil {
			return 0, err
		}

		return uint64(resourceInfo.Size), nil
	}
}

func (t *configurePoolTask) Run(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
) error {

	request := t.state.Request

	imageSize, err := t.checkImage(ctx)
	if err != nil {
		return err
	}

	if !request.UseImageSize {
		imageSize = 0
	}

	return t.storage.ConfigurePool(
		ctx,
		request.ImageId,
		request.ZoneId,
		request.Capacity,
		imageSize,
	)
}

func (t *configurePoolTask) Cancel(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
) error {

	return nil
}

func (t *configurePoolTask) GetMetadata(
	ctx context.Context,
) (proto.Message, error) {
	return &empty.Empty{}, nil
}

func (t *configurePoolTask) GetResponse() proto.Message {
	return &empty.Empty{}
}
