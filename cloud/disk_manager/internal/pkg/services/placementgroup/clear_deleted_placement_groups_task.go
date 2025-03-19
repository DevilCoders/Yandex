package placementgroup

import (
	"context"
	"time"

	"github.com/golang/protobuf/proto"
	"github.com/golang/protobuf/ptypes/empty"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/resources"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks"
)

////////////////////////////////////////////////////////////////////////////////

type clearDeletedPlacementGroupsTask struct {
	storage           resources.Storage
	expirationTimeout time.Duration
	limit             int
}

func (t *clearDeletedPlacementGroupsTask) Init(
	ctx context.Context,
	request proto.Message,
) error {

	return nil
}

func (t *clearDeletedPlacementGroupsTask) Save(
	ctx context.Context,
) ([]byte, error) {

	return proto.Marshal(&empty.Empty{})
}

func (t *clearDeletedPlacementGroupsTask) Load(
	ctx context.Context,
	state []byte,
) error {

	return nil
}

func (t *clearDeletedPlacementGroupsTask) Run(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
) error {

	deletedBefore := time.Now().Add(-t.expirationTimeout)
	return t.storage.ClearDeletedPlacementGroups(
		ctx,
		deletedBefore,
		t.limit,
	)
}

func (t *clearDeletedPlacementGroupsTask) Cancel(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
) error {

	return nil
}

func (t *clearDeletedPlacementGroupsTask) GetMetadata(
	ctx context.Context,
) (proto.Message, error) {

	return &empty.Empty{}, nil
}

func (t *clearDeletedPlacementGroupsTask) GetResponse() proto.Message {
	return &empty.Empty{}
}
