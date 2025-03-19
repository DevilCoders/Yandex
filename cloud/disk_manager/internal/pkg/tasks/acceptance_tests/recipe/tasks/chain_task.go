package tasks

import (
	"context"
	"fmt"
	"math/rand"
	"time"

	"github.com/golang/protobuf/proto"
	"github.com/golang/protobuf/ptypes/empty"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/headers"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/acceptance_tests/recipe/tasks/protos"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/errors"
)

////////////////////////////////////////////////////////////////////////////////

func getRandomDuration(
	min time.Duration,
	max time.Duration,
) time.Duration {

	rand.Seed(time.Now().UnixNano())
	x := min.Microseconds()
	y := max.Microseconds()

	if y <= x {
		return min
	}

	return time.Duration(x+rand.Int63n(y-x)) * time.Microsecond
}

////////////////////////////////////////////////////////////////////////////////

type ChainTask struct {
	scheduler tasks.Scheduler
	state     *protos.ChainTaskState
}

func (t *ChainTask) Init(
	ctx context.Context,
	request proto.Message,
) error {

	typedRequest, ok := request.(*protos.ChainTaskRequest)
	if !ok {
		return fmt.Errorf("invalid request=%v type", request)
	}

	t.state = &protos.ChainTaskState{
		Request: typedRequest,
	}

	return nil
}

func (t *ChainTask) Save(ctx context.Context) ([]byte, error) {
	return proto.Marshal(t.state)
}

func (t *ChainTask) Load(ctx context.Context, state []byte) error {
	t.state = &protos.ChainTaskState{}
	return proto.Unmarshal(state, t.state)
}

func (t *ChainTask) doSomeWork(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
) error {

	t.state.Counter++

	err := execCtx.SaveState(ctx)
	if err != nil {
		return err
	}

	rand.Seed(time.Now().UnixNano())
	<-time.After(getRandomDuration(time.Millisecond, 5*time.Second))

	if rand.Intn(2) == 1 {
		return &errors.InterruptExecutionError{}
	}

	// TODO: should limit overall count of retriable errors.
	if rand.Intn(20) == 1 {
		return &errors.RetriableError{Err: fmt.Errorf("retriable error")}
	}

	return nil
}

func (t *ChainTask) scheduleChild(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
) (string, error) {

	depth := t.state.Request.Depth
	if depth == 0 {
		return "", nil
	}

	return t.scheduler.ScheduleTask(
		headers.SetIncomingIdempotencyKey(ctx, execCtx.GetTaskID()),
		"ChainTask",
		"Chain task",
		&protos.ChainTaskRequest{
			Depth: depth - 1,
		},
		"",
		"",
	)
}

func (t *ChainTask) Run(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
) error {

	err := t.doSomeWork(ctx, execCtx)
	if err != nil {
		return err
	}

	childTaskID, err := t.scheduleChild(ctx, execCtx)
	if err != nil {
		return err
	}

	if len(childTaskID) == 0 {
		return nil
	}

	_, err = t.scheduler.WaitTaskAsync(ctx, execCtx, childTaskID)
	return err
}

func (t *ChainTask) Cancel(
	ctx context.Context,
	execCtx tasks.ExecutionContext,
) error {

	err := t.doSomeWork(ctx, execCtx)
	if err != nil {
		return err
	}

	var childTaskID string
	childTaskID, err = t.scheduleChild(ctx, execCtx)
	if err != nil {
		return err
	}

	if len(childTaskID) == 0 {
		return nil
	}

	_, err = t.scheduler.CancelTask(ctx, childTaskID)
	return err
}

func (t *ChainTask) GetMetadata(
	ctx context.Context,
) (proto.Message, error) {

	return &empty.Empty{}, nil
}

func (t *ChainTask) GetResponse() proto.Message {
	return &empty.Empty{}
}

////////////////////////////////////////////////////////////////////////////////

func NewChainTask(scheduler tasks.Scheduler) *ChainTask {
	return &ChainTask{
		scheduler: scheduler,
	}
}
