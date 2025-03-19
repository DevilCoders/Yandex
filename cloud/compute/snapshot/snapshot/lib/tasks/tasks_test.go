package tasks

import (
	"context"
	"strings"
	"testing"
	"time"

	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/parallellimiter"

	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/storage"

	"go.uber.org/atomic"

	"github.com/prometheus/client_golang/prometheus"
	"github.com/stretchr/testify/mock"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/common"
)

type LockHolderMock string

func (l *LockHolderMock) Close(ctx context.Context) {
	s := string(*l) + " (closed)"
	*l = LockHolderMock(s)
}

func (l *LockHolderMock) IsClosed() bool {
	return strings.HasSuffix(string(*l), " (closed)")
}

func NewLockHolderMock(s string) *LockHolderMock {
	lh := LockHolderMock(s)
	return &lh
}

type taskLimiterStub struct{}

func (taskLimiterStub) WaitQueue(ctx context.Context, descriptor parallellimiter.OperationDescriptor) (
	resCtx context.Context,
	resHolder storage.LockHolder,
	resErr error,
) {
	return ctx, NewLockHolderMock("taskLimiterStub-lock-holder"), nil
}

type taskLimiterMock struct {
	mock.Mock
}

func (tl *taskLimiterMock) WaitQueue(ctx context.Context, od parallellimiter.OperationDescriptor) (
	resCtx context.Context,
	resHolder storage.LockHolder,
	resErr error,
) {
	ret := tl.Called(nil, od)
	if ret.Get(0) != nil {
		resCtx = ret.Get(0).(context.Context)
	}
	if ret.Get(1) != nil {
		resHolder = ret.Get(1).(storage.LockHolder)
	}
	resErr = ret.Error(2)
	return
}

type taskMock struct {
	mock.Mock

	ID            string
	Hash          string
	OperationType string

	OperationCloudID   string
	OperationProjectID string
	SnapshotID         string
}

func (t *taskMock) GetID() string {
	return t.ID
}

func (t *taskMock) GetStatus() *common.TaskStatus {
	ret := t.Called()
	return ret.Get(0).(*common.TaskStatus)
}

func (t *taskMock) Cancel(ctx context.Context) {

}

func (t *taskMock) InitTaskWorkContext(ctx context.Context) context.Context {
	ret := t.Called(nil)
	return ret.Get(0).(context.Context)
}

func (t *taskMock) DoWork(ctx context.Context) (resErr error) {
	ret := t.Called(nil)
	return ret.Error(0)
}

func (t *taskMock) RequestHash() string {
	return t.Hash
}

func (t *taskMock) CheckHeartbeat(err error) error {
	return err
}

func (t *taskMock) GetHeartbeat() time.Duration {
	panic("implement me")
}

func (t *taskMock) Timer(ctx context.Context) *prometheus.Timer {
	return prometheus.NewTimer(nil)
}

func (t *taskMock) GetRequest() common.TaskRequest {
	panic("implement me")
}

func (t *taskMock) GetOperationCloudID() string {
	return t.OperationCloudID
}

func (t *taskMock) GetOperationProjectID() string {
	return t.OperationProjectID
}

func (t *taskMock) Type() string {
	return t.OperationType
}

func (t *taskMock) GetSnapshotID() string {
	return t.SnapshotID
}

func TestTaskManager(t *testing.T) {
	a := assert.New(t)
	m := NewTaskManager(taskLimiterStub{}, "")
	defer m.Close(context.Background())

	task := &MoveContext{request: &common.MoveRequest{
		Src: &common.URLMoveSrc{
			Bucket: "bababa",
			URL:    "bebebe",
		},
		Dst:    nil,
		Params: common.MoveParams{Offset: 213, TaskID: "barabarabara"},
	}}

	a.NoError(m.Add(task))
	a.Error(m.Add(task))
	a.True(m.Delete(task.GetID()))
	a.Equal(1, len(m.deleted))
	task.request.Params.Offset = 1
	a.NoError(m.Add(task))
	a.True(m.Delete(task.GetID()))
	task.request.Params.HeartbeatTimeoutMs = -1
	a.Error(m.Add(task))

	newTask := &MoveContext{request: &common.MoveRequest{
		Src: &common.URLMoveSrc{
			Bucket: "bababa",
			URL:    "bebebe",
		},
		Dst:    nil,
		Params: common.MoveParams{Offset: 213, TaskID: "barabarabara"},
	}}

	a.NoError(m.Add(newTask))
}

func TestListTasks(t *testing.T) {
	a := assert.New(t)
	m := NewTaskManager(taskLimiterStub{}, "")
	defer m.Close(context.Background())

	tasks := []Task{
		&MoveContext{request: &common.MoveRequest{
			Src: &common.URLMoveSrc{
				Bucket: "bababa",
				URL:    "bebebe",
			},
			Dst:    nil,
			Params: common.MoveParams{Offset: 213, TaskID: "barabarabara"},
		}},
		&DeleteContext{request: &common.DeleteRequest{
			Params: common.DeleteParams{
				TaskID: "bereberebere",
			},
		}},
		&DeleteContext{request: &common.DeleteRequest{
			Params: common.DeleteParams{
				TaskID: "zachem-ti-eto-chitaesh",
			},
		}},
	}

	for _, t := range tasks {
		a.NoError(m.Add(t))
	}

	foundIDs := map[string]struct{}{}
	for _, t := range tasks {
		foundIDs[t.GetID()] = struct{}{}
	}

	info := m.ListTasks()

	for _, i := range info {
		delete(foundIDs, i.TaskID)
	}

	a.Equal(0, len(foundIDs))
}

func TestTaskManagerLimiter(t *testing.T) {
	limiter := &taskLimiterMock{}
	at := assert.New(t)
	zoneID := "test-zone-id"
	ctx, cancel := context.WithCancel(context.Background())
	defer cancel()

	opType := "operation-type"
	taskID := "test-task-id"
	cloudID := "test-cloud-id"
	projectID := "test-project-id"
	snapshotID := "test-snapshot-id"

	var taskEndInitStarted = atomic.NewBool(false)
	var taskEndWorkStarted = atomic.NewBool(false)
	taskEndWorkFinish := make(chan struct{})
	task := &taskMock{
		ID:                 taskID,
		OperationType:      opType,
		OperationCloudID:   cloudID,
		OperationProjectID: projectID,
		SnapshotID:         snapshotID,
	}
	task.On("GetStatus").Return(&common.TaskStatus{})
	task.On("InitTaskWorkContext", nil).Return(ctx).Run(func(args mock.Arguments) {
		taskEndInitStarted.Store(true)
	})

	task.On("DoWork", nil).Return(nil).Run(func(args mock.Arguments) {
		taskEndWorkStarted.Store(true)
		<-taskEndWorkFinish
	})

	queueReady := make(chan time.Time)
	lockHolderMock := NewLockHolderMock("queue-lock-holder")
	limiter.On("WaitQueue", nil, parallellimiter.OperationDescriptor{
		Type:       opType,
		ProjectID:  projectID,
		CloudID:    cloudID,
		ZoneID:     zoneID,
		SnapshotID: snapshotID,
	}).WaitUntil(queueReady).Return(
		ctx, lockHolderMock, nil,
	)

	wait := func() { time.Sleep(time.Millisecond * 10) }

	m := NewTaskManager(limiter, zoneID)
	defer m.Close(ctx)

	err := m.Run(ctx, task)
	at.NoError(err)

	wait()
	at.False(taskEndWorkStarted.Load())
	at.False(lockHolderMock.IsClosed())

	close(queueReady)
	wait()
	at.True(taskEndWorkStarted.Load())
	at.False(lockHolderMock.IsClosed())

	close(taskEndWorkFinish)
	wait()
	at.True(lockHolderMock.IsClosed())
}
