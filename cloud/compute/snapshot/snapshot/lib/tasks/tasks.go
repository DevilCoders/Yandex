package tasks

import (
	"errors"
	"fmt"
	"sort"
	"sync"
	"sync/atomic"
	"time"

	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/parallellimiter"

	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/storage"

	"a.yandex-team.ru/cloud/compute/go-common/tracing"

	"github.com/prometheus/client_golang/prometheus"
	"go.uber.org/zap"
	"golang.org/x/net/context"

	log "a.yandex-team.ru/cloud/compute/go-common/logging/ctxlog"
	"a.yandex-team.ru/cloud/compute/go-common/pkg/metriclabels"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/common"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/misc"
)

const (
	minGracePeriod = time.Hour
	maxGracePeriod = time.Hour * 24
	delTasksPeriod = time.Hour

	doneTaskDelPeriod = time.Hour * 48

	OperationTypeUnknown = "unknown"
)

// Task is an interface a snapshot task must provide.
type Task interface {
	GetOperationCloudID() string
	GetOperationProjectID() string
	Type() string

	Cancel(ctx context.Context)
	InitTaskWorkContext(ctx context.Context) context.Context
	DoWork(ctx context.Context) error
	CheckHeartbeat(err error) error

	Timer(ctx context.Context) *prometheus.Timer
	GetRequest() common.TaskRequest
	GetSnapshotID() string

	// Must be fast, call under taskmanager lock
	GetID() string
	GetHeartbeat() time.Duration
	GetStatus() *common.TaskStatus
	RequestHash() string
}

type grave struct {
	t           time.Time
	requestHash string
}

// TaskManager tracks all Tasks.
type TaskManager struct {
	taskLimiter TaskLimiter
	zoneID      string

	tasks map[string]Task
	lock  sync.RWMutex

	deleted map[string]grave

	done    chan struct{}
	working bool

	currentWaitingTasks uint64
}

type TaskLimiter interface {
	WaitQueue(ctx context.Context, od parallellimiter.OperationDescriptor) (
		resCtx context.Context,
		resHolder storage.LockHolder,
		resErr error,
	)
}

// NewTaskManager returns new TaskManager instance.
func NewTaskManager(taskLimiter TaskLimiter, zoneID string) *TaskManager {
	tm := &TaskManager{
		tasks:               make(map[string]Task),
		deleted:             make(map[string]grave),
		done:                make(chan struct{}),
		working:             true,
		taskLimiter:         taskLimiter,
		zoneID:              zoneID,
		currentWaitingTasks: 0,
	}
	go tm.cycleClearDeletedTasks()
	return tm
}

// Add adds a task to tracking.
func (tm *TaskManager) Add(tc Task) error {
	tm.lock.Lock()
	defer tm.lock.Unlock()

	if _, ok := tm.tasks[tc.GetID()]; ok {
		return errors.New("task with same id already present")
	}
	if found, ok := tm.deleted[tc.GetID()]; ok && found.requestHash != tc.RequestHash() {
		return errors.New("tasks with same id and non-similar request present")
	}

	tm.tasks[tc.GetID()] = tc
	misc.CurrentInManagerTasks.Inc()
	return nil
}

// Get returns a task.
func (tm *TaskManager) Get(taskID string) Task {
	tm.lock.RLock()
	defer tm.lock.RUnlock()

	return tm.tasks[taskID]
}

func taskGracePeriod(heartbeat time.Duration) time.Time {
	heartbeat *= 3
	if heartbeat < minGracePeriod {
		heartbeat = minGracePeriod
	}
	if heartbeat > maxGracePeriod {
		heartbeat = maxGracePeriod
	}
	return time.Now().Add(heartbeat)
}

// Delete removed task from tracking.
func (tm *TaskManager) Delete(taskID string) bool {
	tm.lock.Lock()
	defer tm.lock.Unlock()

	if _, ok := tm.tasks[taskID]; !ok {
		return false
	}

	tm.deleted[taskID] = grave{
		t:           taskGracePeriod(tm.tasks[taskID].GetHeartbeat()),
		requestHash: tm.tasks[taskID].RequestHash(),
	}

	delete(tm.tasks, taskID)
	misc.CurrentInManagerTasks.Dec()

	return true
}

func (tm *TaskManager) clearDeletedTasks() {
	tm.lock.Lock()
	defer tm.lock.Unlock()

	now := time.Now()
	for id, g := range tm.deleted {
		if g.t.Before(now) {
			delete(tm.deleted, id)
		}
	}
}

func (tm *TaskManager) localTaskList() []Task {
	tm.lock.Lock()
	defer tm.lock.Unlock()

	list := make([]Task, 0, len(tm.tasks))
	for _, task := range tm.tasks {
		list = append(list, task)
	}
	return list
}

func (tm *TaskManager) deleteCompletedTasks() {
	taskList := tm.localTaskList()

	cleanUntil := time.Now().Add(-doneTaskDelPeriod)
	for _, g := range taskList {
		st := g.GetStatus()
		if st.Finished && st.FinishedAt != nil && st.FinishedAt.Before(cleanUntil) {
			tm.Delete(g.GetID())
		}
	}
}

func (tm *TaskManager) cycleClearDeletedTasks() {
	ticker := time.NewTicker(delTasksPeriod)
	for {
		select {
		case <-tm.done:
			ticker.Stop()
			return
		case <-ticker.C:
			tm.clearDeletedTasks()
			tm.deleteCompletedTasks()
		}
	}
}

// Run adds task to tracking and starts its execution.
func (tm *TaskManager) Run(ctx context.Context, tc Task) error {
	span, ctx := tracing.StartSpanFromContext(ctx, "run")
	defer func() { tracing.Finish(span, nil) }()

	if tc.GetID() == "" {
		log.G(ctx).Error("Run failed. Empty task ID.", zap.Error(misc.ErrInvalidTaskID))
		return misc.ErrInvalidTaskID
	}

	readyChan := make(chan struct{})
	data := metriclabels.Get(ctx)
	data.TaskID = tc.GetID()
	asyncCtx := misc.WithNoCancel(ctx)

	if err := tm.Add(tc); err != nil {
		log.G(ctx).Error("Run failed", zap.Error(err))
		//tc.Cancel()
		return misc.ErrDuplicateTaskID
	}

	go func() {
		span, asyncCtx := tracing.FollowSpanFromContext(asyncCtx, "background-task")
		defer func() { tracing.Finish(span, nil) }()

		t := tc.Timer(asyncCtx)
		defer t.ObserveDuration()

		// taskCtx must not be use after tc.DoWork
		taskCtx := tc.InitTaskWorkContext(asyncCtx)
		close(readyChan)

		od := parallellimiter.OperationDescriptor{
			Type:       tc.Type(),
			ProjectID:  tc.GetOperationProjectID(),
			CloudID:    tc.GetOperationCloudID(),
			ZoneID:     tm.zoneID,
			SnapshotID: tc.GetSnapshotID(),
		}

		misc.CurrentWaitingTasks.Inc()
		atomic.AddUint64(&tm.currentWaitingTasks, 1)
		queueCtx, lockHolder, resErr := tm.taskLimiter.WaitQueue(taskCtx, od)
		log.DebugErrorCtx(taskCtx, resErr, "Wait task queue")
		misc.CurrentWaitingTasks.Dec()
		atomic.AddUint64(&tm.currentWaitingTasks, ^uint64(0))

		if resErr == nil {
			defer lockHolder.Close(asyncCtx)
		} else {
			tc.Cancel(asyncCtx)
		}

		if resErr == nil {
			misc.CurrentRunningTasks.Inc()
			defer misc.CurrentRunningTasks.Dec()
			log.G(ctx).Debug("calling Task.Endwork")
			resErr = tc.DoWork(queueCtx)
		}

		checkedErr := tc.CheckHeartbeat(resErr)
		log.G(ctx).Debug("Check error by heartbeat", zap.NamedError("resErr", resErr), zap.NamedError("checkedErr", checkedErr))
		resErr = checkedErr

		switch resErr {
		case misc.ErrHeartbeatTimeout:
			log.G(asyncCtx).Error("task was ended from heartbeatTimeout", zap.String("task-id", tc.GetID()), zap.Any("task-status", tc.GetStatus()))
			misc.TaskRunHeartbeatTimeout.Inc()
			tm.Delete(tc.GetID())
		case nil:
			log.G(asyncCtx).Info("task done successfully", zap.String("task-id", tc.GetID()))
			misc.TaskRunNoError.Inc()
		default:
			if common.IsSnapshotPublicError(resErr) {
				misc.TaskRunPublicError.Inc()
			} else {
				misc.TaskRunUnknownError.Inc()
			}
			log.G(asyncCtx).Error("task failed with error", zap.String("task-id", tc.GetID()), zap.Any("task-status", tc.GetStatus()), zap.Error(resErr))
		}

	}()

	<-readyChan

	return nil
}

// Lists all current tasks
func (tm *TaskManager) ListTasks() []*common.TaskInfo {
	taskList := tm.localTaskList()

	res := make([]*common.TaskInfo, len(tm.tasks))
	for i, t := range taskList {
		res[i] = &common.TaskInfo{
			TaskID: t.GetID(),
			Status: *t.GetStatus(),
		}
	}
	sort.Slice(res, func(i, j int) bool {
		return res[i].TaskID > res[j].TaskID
	})
	return res
}

// Close closes task manager.
func (tm *TaskManager) Close(ctx context.Context) {
	tm.lock.Lock()
	defer tm.lock.Unlock()

	if tm.working {
		close(tm.done)
		tm.working = false
	}

	for taskID, task := range tm.tasks {
		task.Cancel(ctx)
		if status := task.GetStatus(); status.Error != nil {
			log.G(ctx).Error("Task failed on Cancel.", zap.String("task_id", taskID),
				zap.Error(status.Error))
		}
		delete(tm.tasks, taskID)
	}

	for id := range tm.deleted {
		delete(tm.deleted, id)
	}
}

// Monitoring check
func (tm *TaskManager) Check(ctx context.Context) error {
	waitingTasks := atomic.LoadUint64(&tm.currentWaitingTasks)
	if waitingTasks > 20 {
		return fmt.Errorf("too many waiting tasks: %d", waitingTasks)
	}

	return nil
}
