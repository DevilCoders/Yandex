package disks

import (
	"context"
	"errors"
	"fmt"

	"github.com/golang/protobuf/proto"
	"github.com/golang/protobuf/ptypes/empty"

	"a.yandex-team.ru/cloud/disk_manager/api/yandex/cloud/priv/disk_manager/v1"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/disks/protos"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks"
)

////////////////////////////////////////////////////////////////////////////////

type migrateDiskTask struct {
	state *protos.MigrateDiskTaskState
}

func (t *migrateDiskTask) Init(
	_ context.Context,
	request proto.Message,
) error {

	typedRequest, ok := request.(*protos.MigrateDiskRequest)
	if !ok {
		return fmt.Errorf("invalid request=%v type", request)
	}

	t.state = &protos.MigrateDiskTaskState{
		Request: typedRequest,
	}

	return nil
}

func (t *migrateDiskTask) Save(_ context.Context) ([]byte, error) {
	return proto.Marshal(t.state)
}

func (t *migrateDiskTask) Load(_ context.Context, state []byte) error {
	t.state = &protos.MigrateDiskTaskState{}
	return proto.Unmarshal(state, t.state)
}

func (t *migrateDiskTask) Run(
	_ context.Context,
	_ tasks.ExecutionContext,
) error {

	// TODO: create dst disk.
	// TODO: add stages of migration.
	// TODO: add disk data syncing.

	return errors.New("not implemented")
}

func (t *migrateDiskTask) Cancel(
	_ context.Context,
	_ tasks.ExecutionContext,
) error {

	// TODO: implement.
	return nil
}

func (t *migrateDiskTask) GetMetadata(
	_ context.Context,
) (proto.Message, error) {

	// TODO: implement.
	return &disk_manager.MigrateDiskMetadata{}, nil
}

func (t *migrateDiskTask) GetResponse() proto.Message {
	return &empty.Empty{}
}
