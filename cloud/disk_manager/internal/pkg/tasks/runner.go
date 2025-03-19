package tasks

import (
	"context"
	"fmt"
	"math/rand"
	"sync"
	"sync/atomic"
	"time"

	grpc_status "google.golang.org/grpc/status"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/headers"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/logging"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/monitoring/metrics"
	tasks_config "a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/config"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/errors"
	tasks_storage "a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/storage"
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

func fillError(
	ctx context.Context,
	taskState *tasks_storage.TaskState,
	e error,
) {

	status := grpc_status.Convert(e)
	taskState.ErrorCode = status.Code()
	taskState.ErrorMessage = status.Message()
	taskState.ErrorSilent = errors.IsSilent(e)

	detailedError := &errors.DetailedError{}
	if errors.As(e, &detailedError) {
		taskState.ErrorDetails = detailedError.Details
	}
}

////////////////////////////////////////////////////////////////////////////////

type executionContext struct {
	task           Task
	storage        tasks_storage.Storage
	taskState      tasks_storage.TaskState
	taskStateMutex sync.Mutex
}

// HACK from https://github.com/stretchr/testify/pull/694/files for avoid fake race detection
func (c *executionContext) String() string {
	return fmt.Sprintf("%[1]T<%[1]p>", c)
}

func (c *executionContext) SaveState(ctx context.Context) error {
	state, err := c.task.Save(ctx)
	if err != nil {
		return err
	}

	return c.updateState(ctx, func(taskState tasks_storage.TaskState) tasks_storage.TaskState {
		logging.Debug(
			ctx,
			"executionContext.SaveState %v",
			taskState.ID,
		)

		taskState.State = state
		return taskState
	})
}

func (c *executionContext) GetTaskType() string {
	c.taskStateMutex.Lock()
	defer c.taskStateMutex.Unlock()
	return c.taskState.TaskType
}

func (c *executionContext) GetTaskID() string {
	c.taskStateMutex.Lock()
	defer c.taskStateMutex.Unlock()
	return c.taskState.ID
}

func (c *executionContext) GetRetriableErrorCount() uint64 {
	c.taskStateMutex.Lock()
	defer c.taskStateMutex.Unlock()
	return c.taskState.RetriableErrorCount
}

func (c *executionContext) AddTaskDependency(
	ctx context.Context,
	taskID string,
) error {

	state, err := c.task.Save(ctx)
	if err != nil {
		return err
	}

	return c.updateState(ctx, func(taskState tasks_storage.TaskState) tasks_storage.TaskState {
		logging.Debug(
			ctx,
			"executionContext.addTaskDependency of %v on %v",
			taskState.ID,
			taskID,
		)

		taskState.State = state
		taskState.Dependencies.Add(taskID)
		return taskState
	})
}

func (c *executionContext) SetEstimate(estimatedDuration time.Duration) {
	c.taskStateMutex.Lock()
	defer c.taskStateMutex.Unlock()

	if c.taskState.EstimatedTime.Before(c.taskState.CreatedAt) {
		c.taskState.EstimatedTime = c.taskState.CreatedAt.Add(estimatedDuration)
	}
}

////////////////////////////////////////////////////////////////////////////////

func (c *executionContext) updateState(
	ctx context.Context,
	f func(tasks_storage.TaskState) tasks_storage.TaskState,
) error {

	c.taskStateMutex.Lock()
	defer c.taskStateMutex.Unlock()

	taskState := f(c.taskState)
	taskState = taskState.DeepCopy()

	taskState.ModifiedAt = time.Now()

	newTaskState, err := c.storage.UpdateTask(ctx, taskState)
	if err != nil {
		return err
	}

	c.taskState = newTaskState
	return nil
}

func (c *executionContext) incrementRetriableErrorCount(
	ctx context.Context,
) error {

	state, err := c.task.Save(ctx)
	if err != nil {
		return err
	}

	return c.updateState(ctx, func(taskState tasks_storage.TaskState) tasks_storage.TaskState {
		taskState.State = state
		taskState.RetriableErrorCount++
		return taskState
	})
}

func (c *executionContext) setError(
	ctx context.Context,
	e error,
) error {

	state, err := c.task.Save(ctx)
	if err != nil {
		return err
	}

	err = c.updateState(ctx, func(taskState tasks_storage.TaskState) tasks_storage.TaskState {
		taskState.State = state
		taskState.Status = tasks_storage.TaskStatusReadyToCancel
		fillError(ctx, &taskState, e)
		return taskState
	})
	if err != nil {
		errors.LogError(
			ctx,
			err,
			"executionContext.setError failed to commit non retriable error for %v with id=%v",
			c.GetTaskType(),
			c.GetTaskID(),
		)
		return err
	}

	errors.LogError(
		ctx,
		e,
		"executionContext.setError commited fatal error for %v with id=%v",
		c.GetTaskType(),
		c.GetTaskID(),
	)
	return nil
}

func (c *executionContext) setNonCancellableError(
	ctx context.Context,
	e error,
) error {

	state, err := c.task.Save(ctx)
	if err != nil {
		return err
	}

	return c.updateState(ctx, func(taskState tasks_storage.TaskState) tasks_storage.TaskState {
		taskState.State = state
		taskState.Status = tasks_storage.TaskStatusCancelled
		fillError(ctx, &taskState, e)
		return taskState
	})
}

func (c *executionContext) setFinished(ctx context.Context) error {
	state, err := c.task.Save(ctx)
	if err != nil {
		return err
	}

	return c.updateState(ctx, func(taskState tasks_storage.TaskState) tasks_storage.TaskState {
		taskState.State = state
		taskState.Status = tasks_storage.TaskStatusFinished
		return taskState
	})
}

func (c *executionContext) setCancelled(ctx context.Context) error {
	state, err := c.task.Save(ctx)
	if err != nil {
		return err
	}

	return c.updateState(ctx, func(taskState tasks_storage.TaskState) tasks_storage.TaskState {
		taskState.State = state
		taskState.Status = tasks_storage.TaskStatusCancelled
		return taskState
	})
}

func (c *executionContext) ping(ctx context.Context) error {
	return c.updateState(ctx, func(taskState tasks_storage.TaskState) tasks_storage.TaskState {
		return taskState
	})
}

////////////////////////////////////////////////////////////////////////////////

func makeExecutionContext(
	task Task,
	storage tasks_storage.Storage,
	taskState tasks_storage.TaskState,
) *executionContext {

	return &executionContext{
		task:      task,
		storage:   storage,
		taskState: taskState,
	}
}

////////////////////////////////////////////////////////////////////////////////

type taskHandle struct {
	task    tasks_storage.TaskInfo
	onClose func()
}

func (h *taskHandle) close() {
	h.onClose()
}

type channel struct {
	handle chan taskHandle
}

func (c *channel) receive(ctx context.Context) (taskHandle, error) {
	select {
	case <-ctx.Done():
		return taskHandle{}, ctx.Err()
	case handle, more := <-c.handle:
		if !more {
			return taskHandle{}, fmt.Errorf("channel.handle is closed")
		}

		return handle, nil
	}
}

func (c *channel) send(handle taskHandle) bool {
	select {
	case c.handle <- handle:
		return true
	default:
		handle.close()
		return false
	}
}

func (c *channel) close() {
	close(c.handle)
}

////////////////////////////////////////////////////////////////////////////////

type runner interface {
	receiveTask(ctx context.Context) (taskHandle, error)
	lockTask(context.Context, tasks_storage.TaskInfo) (tasks_storage.TaskState, error)
	executeTask(context.Context, *executionContext, Task)
	tryExecutingTask(context.Context, tasks_storage.TaskInfo) error
}

////////////////////////////////////////////////////////////////////////////////

type runnerForRun struct {
	storage                tasks_storage.Storage
	registry               *Registry
	metrics                runnerMetrics
	channel                *channel
	pingPeriod             time.Duration
	host                   string
	id                     string
	maxRetriableErrorCount uint64
}

func (r *runnerForRun) receiveTask(
	ctx context.Context,
) (taskHandle, error) {

	return r.channel.receive(ctx)
}

func (r *runnerForRun) lockTask(
	ctx context.Context,
	taskInfo tasks_storage.TaskInfo,
) (tasks_storage.TaskState, error) {

	logging.Debug(
		ctx,
		"runnerForRun.lockTask started with taskInfo=%v, host=%v, runner id=%v",
		taskInfo,
		r.host,
		r.id,
	)
	return r.storage.LockTaskToRun(ctx, taskInfo, time.Now(), r.host, r.id)
}

func (r *runnerForRun) executeTask(
	ctx context.Context,
	execCtx *executionContext,
	task Task,
) {

	taskType := execCtx.GetTaskType()
	taskID := execCtx.GetTaskID()

	logging.Debug(
		ctx,
		"runnerForRun.executeTask started %v with id=%v",
		taskType,
		taskID,
	)

	err := task.Run(ctx, execCtx)

	if ctx.Err() != nil {
		logging.Info(
			ctx,
			"runnerForRun.executeTask cancelled %v with id=%v",
			taskType,
			taskID,
		)
		// If context was cancelled, just return.
		return
	}

	if err == nil {
		// If there was no error, task has completed successfully.
		err = execCtx.setFinished(ctx)
		if err != nil {
			errors.LogError(
				ctx,
				err,
				"runnerForRun.executeTask failed to commit finishing for %v with id=%v",
				taskType,
				taskID,
			)
			r.metrics.OnError(err)
		}
		return
	}

	errors.LogError(
		ctx,
		err,
		"runnerForRun.executeTask got error for %v with id=%v",
		taskType,
		taskID,
	)
	r.metrics.OnExecutionError(err)

	if errors.Is(err, &errors.WrongGenerationError{}) ||
		errors.Is(err, &errors.InterruptExecutionError{}) {
		return
	}

	if errors.Is(err, &errors.NonCancellableError{}) {
		err = execCtx.setNonCancellableError(ctx, err)
		if err != nil {
			errors.LogError(
				ctx,
				err,
				"runnerForRun.executeTask failed to commit non cancellable error for %v with id=%v",
				taskType,
				taskID,
			)
			r.metrics.OnError(err)
		}
		return
	}

	if errors.Is(err, &errors.NonRetriableError{}) {
		err = execCtx.setError(ctx, err)
		if err != nil {
			r.metrics.OnError(err)
		}
		return
	}

	retriableError := &errors.RetriableError{}
	if errors.As(err, &retriableError) {
		if !retriableError.IgnoreRetryLimit &&
			execCtx.GetRetriableErrorCount() >= r.maxRetriableErrorCount {

			errors.LogError(
				ctx,
				err,
				"runnerForRun.executeTask retriable error count exceeded for %v with id=%v",
				taskType,
				taskID,
			)
			// Wrap into NonRetriableError to indicate failure.
			wrappedErr := &errors.NonRetriableError{Err: err}
			r.metrics.OnExecutionError(wrappedErr)

			err = execCtx.setError(ctx, err)
			if err != nil {
				r.metrics.OnError(err)
			}
			return
		}

		err = execCtx.incrementRetriableErrorCount(ctx)
		if err != nil {
			errors.LogError(
				ctx,
				err,
				"runnerForRun.executeTask failed to increment retriable error count for %v with id=%v",
				taskType,
				taskID,
			)
			r.metrics.OnError(err)
		}
		return
	}

	// This is a significant error, task must be cancelled
	// to undo the damage.
	err = execCtx.setError(ctx, err)
	if err != nil {
		r.metrics.OnError(err)
	}
}

func (r *runnerForRun) tryExecutingTask(
	ctx context.Context,
	taskInfo tasks_storage.TaskInfo,
) error {

	return tryExecutingTask(
		ctx,
		r.storage,
		r.registry,
		r.metrics,
		r.pingPeriod,
		r,
		taskInfo,
	)
}

////////////////////////////////////////////////////////////////////////////////

type runnerForCancel struct {
	storage    tasks_storage.Storage
	registry   *Registry
	metrics    runnerMetrics
	channel    *channel
	pingPeriod time.Duration
	host       string
	id         string
}

func (r *runnerForCancel) receiveTask(
	ctx context.Context,
) (taskHandle, error) {

	return r.channel.receive(ctx)
}

func (r *runnerForCancel) lockTask(
	ctx context.Context,
	taskInfo tasks_storage.TaskInfo,
) (tasks_storage.TaskState, error) {

	logging.Debug(
		ctx,
		"runnerForCancel.lockTask started with taskInfo=%v, host=%v, runner id=%v",
		taskInfo,
		r.host,
		r.id,
	)
	return r.storage.LockTaskToCancel(ctx, taskInfo, time.Now(), r.host, r.id)
}

func (r *runnerForCancel) executeTask(
	ctx context.Context,
	execCtx *executionContext,
	task Task,
) {

	taskType := execCtx.GetTaskType()
	taskID := execCtx.GetTaskID()

	logging.Debug(
		ctx,
		"runnerForCancel.executeTask started for %v with id=%v",
		taskType,
		taskID,
	)

	err := task.Cancel(ctx, execCtx)
	if err != nil {
		errors.LogError(
			ctx,
			err,
			"runnerForCancel.executeTask got error for %v with id=%v",
			taskType,
			taskID,
		)
		r.metrics.OnExecutionError(err)
		// Treat any errors (other than NonRetriableError or NonCancellableError)
		// as retriable.
		if !errors.Is(err, &errors.NonRetriableError{}) &&
			!errors.Is(err, &errors.NonCancellableError{}) {
			// In case of any retriable errors, we must not drop the cancel job.
			// Let someone else deal with it.
			// TODO: Maybe do need to drop after Nth retry?
			return
		}
	}

	logging.Debug(
		ctx,
		"runnerForCancel.executeTask completed for %v with id=%v",
		taskType,
		taskID,
	)

	// If we have no error or have non retriable error, the cancellation has
	// completed.
	err = execCtx.setCancelled(ctx)
	if err != nil {
		errors.LogError(
			ctx,
			err,
			"runnerForCancel.executeTask failed to commit cancellation for %v with id=%v",
			taskType,
			taskID,
		)
		r.metrics.OnError(err)
	}
}

func (r *runnerForCancel) tryExecutingTask(
	ctx context.Context,
	taskInfo tasks_storage.TaskInfo,
) error {

	return tryExecutingTask(
		ctx,
		r.storage,
		r.registry,
		r.metrics,
		r.pingPeriod,
		r,
		taskInfo,
	)
}

////////////////////////////////////////////////////////////////////////////////

func taskPinger(
	ctx context.Context,
	execCtx *executionContext,
	pingPeriod time.Duration,
	onError func(),
) {

	logging.Debug(
		ctx,
		"taskPinger started for %v",
		execCtx.GetTaskID(),
	)

	for {
		err := execCtx.ping(ctx)
		// Pinger being cancelled does not constitute an error.
		if err != nil && ctx.Err() == nil {
			errors.LogError(
				ctx,
				err,
				"taskPinger failed to ping %v",
				execCtx.GetTaskID(),
			)
			onError()
			return
		}
		select {
		case <-ctx.Done():
			logging.Debug(
				ctx,
				"taskPinger stopped for %v",
				execCtx.GetTaskID(),
			)
			return
		case <-time.After(pingPeriod):
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

func tryExecutingTask(
	ctx context.Context,
	storage tasks_storage.Storage,
	registry *Registry,
	runnerMetrics runnerMetrics,
	pingPeriod time.Duration,
	runner runner,
	taskInfo tasks_storage.TaskInfo,
) error {

	taskState, err := runner.lockTask(ctx, taskInfo)
	if err != nil {
		errors.LogError(
			ctx,
			err,
			"tryExecutingTask failed to lock task %v",
			taskInfo,
		)
		runnerMetrics.OnError(err)
		return err
	}

	task, err := registry.CreateTask(ctx, taskState.TaskType)
	if err != nil {
		errors.LogError(
			ctx,
			err,
			"tryExecutingTask failed to construct task %v",
			taskInfo,
		)
		runnerMetrics.OnError(err)
		// If we've failed to construct this task probably because our
		// version does not support it yet.
		return err
	}

	err = task.Load(ctx, taskState.State)
	if err != nil {
		errors.LogError(
			ctx,
			err,
			"tryExecutingTask failed to Load task %v",
			taskInfo,
		)
		runnerMetrics.OnError(err)
		// This task might be corrupted.
		return err
	}

	runCtx, cancelRun := context.WithCancel(ctx)
	runCtx = headers.Append(runCtx, taskState.Metadata.Vals())
	// All derived tasks should be pinned to the same storage folder.
	runCtx = setStorageFolderForTasksPinning(runCtx, taskState.StorageFolder)
	runCtx = logging.SetLoggingFields(runCtx)

	execCtx := makeExecutionContext(task, storage, taskState)

	pingCtx, cancelPing := context.WithCancel(ctx)
	go taskPinger(pingCtx, execCtx, pingPeriod, cancelRun)
	defer cancelPing()

	runnerMetrics.OnExecutionStarted(taskState)
	runner.executeTask(runCtx, execCtx, task)
	runnerMetrics.OnExecutionStopped()

	return nil
}

////////////////////////////////////////////////////////////////////////////////

type lister struct {
	listTasks             func(ctx context.Context, limit uint64) ([]tasks_storage.TaskInfo, error)
	channels              []*channel
	pollForTasksPeriodMin time.Duration
	pollForTasksPeriodMax time.Duration
	inflightTasks         sync.Map
	inflightTaskCount     int32
}

func newLister(
	ctx context.Context,
	listTasks func(ctx context.Context, limit uint64) ([]tasks_storage.TaskInfo, error),
	channelsCount uint64,
	pollForTasksPeriodMin time.Duration,
	pollForTasksPeriodMax time.Duration,
) *lister {

	lister := &lister{
		listTasks:             listTasks,
		channels:              make([]*channel, channelsCount),
		pollForTasksPeriodMin: pollForTasksPeriodMin,
		pollForTasksPeriodMax: pollForTasksPeriodMax,
	}
	for i := 0; i < len(lister.channels); i++ {
		lister.channels[i] = &channel{
			handle: make(chan taskHandle),
		}
	}
	go lister.loop(ctx)
	return lister
}

func (l *lister) loop(
	ctx context.Context,
) {

	defer func() {
		for _, c := range l.channels {
			c.close()
		}
		logging.Debug(ctx, "lister stopped")
	}()

	wait := func() error {
		select {
		case <-ctx.Done():
			return ctx.Err()
		case <-time.After(
			getRandomDuration(
				l.pollForTasksPeriodMin,
				l.pollForTasksPeriodMax,
			)):
		}

		return nil
	}

	for {
		inflightTaskCount := atomic.LoadInt32(&l.inflightTaskCount)

		if int32(len(l.channels)) <= inflightTaskCount {
			err := wait()
			if err != nil {
				return
			}

			continue
		}

		limit := int32(len(l.channels)) - inflightTaskCount

		tasks, err := l.listTasks(ctx, uint64(limit))
		if err == nil {
			logging.Debug(ctx, "lister listed %v tasks", len(tasks))

			// HACK: Random shuffle tasks in order to reduce contention between
			// nodes.
			rand.Seed(time.Now().UnixNano())
			rand.Shuffle(
				len(tasks),
				func(i, j int) { tasks[i], tasks[j] = tasks[j], tasks[i] },
			)

			taskIdx := 0
			for _, channel := range l.channels {
				if taskIdx >= len(tasks) {
					break
				}

				task := tasks[taskIdx]

				_, loaded := l.inflightTasks.LoadOrStore(
					task.ID,
					struct{}{},
				)
				if loaded {
					taskIdx++
					// This task is already executing, drop it.
					continue
				}
				atomic.AddInt32(&l.inflightTaskCount, 1)

				handle := taskHandle{
					task: task,
					onClose: func() {
						l.inflightTasks.Delete(task.ID)
						atomic.AddInt32(&l.inflightTaskCount, -1)
					},
				}
				if channel.send(handle) {
					taskIdx++
				}
			}
		}

		err = wait()
		if err != nil {
			return
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

func runnerLoop(
	ctx context.Context,
	storage tasks_storage.Storage,
	registry *Registry,
	runner runner,
) {

	logging.Debug(ctx, "runnerLoop started")

	for {
		handle, err := runner.receiveTask(ctx)
		if err != nil {
			logging.Warn(ctx, "runnerLoop iteration stopped: %v", err)
			return
		}

		logging.Debug(ctx, "runnerLoop iteration trying %v", handle.task)
		err = runner.tryExecutingTask(ctx, handle.task)
		if err == nil {
			logging.Debug(ctx, "runnerLoop iteration completed successfully %v", handle.task)
		}
		handle.close()

		if ctx.Err() != nil {
			logging.Debug(ctx, "runnerLoop iteration stopped: ctx cancelled")
			return
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

func startRunner(
	ctx context.Context,
	storage tasks_storage.Storage,
	registry *Registry,
	runnerMetricsRegistry metrics.Registry,
	channelForRun *channel,
	channelForCancel *channel,
	pingPeriod time.Duration,
	hangingTaskTimeout time.Duration,
	host string,
	idForRun string,
	idForCancel string,
	maxRetriableErrorCount uint64,
) error {

	// TODO: More granular control on runners and cancellers.

	go runnerLoop(ctx, storage, registry, &runnerForRun{
		storage:                storage,
		registry:               registry,
		metrics:                newRunnerMetrics(ctx, runnerMetricsRegistry, hangingTaskTimeout),
		channel:                channelForRun,
		pingPeriod:             pingPeriod,
		host:                   host,
		id:                     idForRun,
		maxRetriableErrorCount: maxRetriableErrorCount,
	})

	go runnerLoop(ctx, storage, registry, &runnerForCancel{
		storage:    storage,
		registry:   registry,
		metrics:    newRunnerMetrics(ctx, runnerMetricsRegistry, hangingTaskTimeout),
		channel:    channelForCancel,
		pingPeriod: pingPeriod,
		host:       host,
		id:         idForCancel,
	})

	return nil
}

func startRunners(
	ctx context.Context,
	runnerCount uint64,
	storage tasks_storage.Storage,
	registry *Registry,
	runnerMetricsRegistry metrics.Registry,
	channelsForRun []*channel,
	channelsForCancel []*channel,
	pingPeriod time.Duration,
	hangingTaskTimeout time.Duration,
	host string,
	maxRetriableErrorCount uint64,
) error {

	for i := uint64(0); i < runnerCount; i++ {
		err := startRunner(
			ctx,
			storage,
			registry,
			runnerMetricsRegistry,
			channelsForRun[i],
			channelsForCancel[i],
			pingPeriod,
			hangingTaskTimeout,
			host,
			fmt.Sprintf("run_%v", i),
			fmt.Sprintf("cancel_%v", i),
			maxRetriableErrorCount,
		)
		if err != nil {
			return fmt.Errorf("failed to start runner #%d: %w", i, err)
		}
	}

	return nil
}

func startStalkingRunners(
	ctx context.Context,
	runnerCount uint64,
	storage tasks_storage.Storage,
	registry *Registry,
	runnerMetricsRegistry metrics.Registry,
	channelsForRun []*channel,
	channelsForCancel []*channel,
	pingPeriod time.Duration,
	hangingTaskTimeout time.Duration,
	host string,
	maxRetriableErrorCount uint64,
) error {

	for i := uint64(0); i < runnerCount; i++ {
		err := startRunner(
			ctx,
			storage,
			registry,
			runnerMetricsRegistry,
			channelsForRun[i],
			channelsForCancel[i],
			pingPeriod,
			hangingTaskTimeout,
			host,
			fmt.Sprintf("stalker_run_%v", i),
			fmt.Sprintf("stalker_cancel_%v", i),
			maxRetriableErrorCount,
		)
		if err != nil {
			return fmt.Errorf("failed to start stalking runner #%d: %w", i, err)
		}
	}

	return nil
}

func StartRunners(
	ctx context.Context,
	storage tasks_storage.Storage,
	registry *Registry,
	runnerMetricsRegistry metrics.Registry,
	config *tasks_config.TasksConfig,
	host string,
) error {

	pollForTasksPeriodMin, err := time.ParseDuration(config.GetPollForTasksPeriodMin())
	if err != nil {
		return err
	}

	pollForTasksPeriodMax, err := time.ParseDuration(config.GetPollForTasksPeriodMax())
	if err != nil {
		return err
	}

	if pollForTasksPeriodMin > pollForTasksPeriodMax {
		return fmt.Errorf(
			"pollForTasksPeriodMin should not be greater than pollForTasksPeriodMax",
		)
	}

	pollForStallingTasksPeriodMin, err := time.ParseDuration(config.GetPollForStallingTasksPeriodMin())
	if err != nil {
		return err
	}

	pollForStallingTasksPeriodMax, err := time.ParseDuration(config.GetPollForStallingTasksPeriodMax())
	if err != nil {
		return err
	}

	if pollForStallingTasksPeriodMin > pollForStallingTasksPeriodMax {
		return fmt.Errorf(
			"pollForStallingTasksPeriodMin should not be greater than pollForStallingTasksPeriodMax",
		)
	}

	pingPeriod, err := time.ParseDuration(config.GetTaskPingPeriod())
	if err != nil {
		return err
	}

	hangingTaskTimeout, err := time.ParseDuration(config.GetHangingTaskTimeout())
	if err != nil {
		return err
	}

	listerReadyToRun := newLister(
		ctx,
		storage.ListTasksReadyToRun,
		config.GetRunnersCount(),
		pollForTasksPeriodMin,
		pollForTasksPeriodMax,
	)
	listerReadyToCancel := newLister(
		ctx,
		storage.ListTasksReadyToCancel,
		config.GetRunnersCount(),
		pollForTasksPeriodMin,
		pollForTasksPeriodMax,
	)

	err = startRunners(
		ctx,
		config.GetRunnersCount(),
		storage,
		registry,
		runnerMetricsRegistry,
		listerReadyToRun.channels,
		listerReadyToCancel.channels,
		pingPeriod,
		hangingTaskTimeout,
		host,
		config.GetMaxRetriableErrorCount(),
	)
	if err != nil {
		return err
	}

	listerStallingWhileRunning := newLister(
		ctx,
		func(ctx context.Context, limit uint64) ([]tasks_storage.TaskInfo, error) {
			return storage.ListTasksStallingWhileRunning(ctx, host, limit)
		},
		config.GetStalkingRunnersCount(),
		pollForStallingTasksPeriodMin,
		pollForStallingTasksPeriodMax,
	)
	listerStallingWhileCancelling := newLister(
		ctx,
		func(ctx context.Context, limit uint64) ([]tasks_storage.TaskInfo, error) {
			return storage.ListTasksStallingWhileCancelling(ctx, host, limit)
		},
		config.GetStalkingRunnersCount(),
		pollForStallingTasksPeriodMin,
		pollForStallingTasksPeriodMax,
	)

	return startStalkingRunners(
		ctx,
		config.GetStalkingRunnersCount(),
		storage,
		registry,
		runnerMetricsRegistry,
		listerStallingWhileRunning.channels,
		listerStallingWhileCancelling.channels,
		pingPeriod,
		hangingTaskTimeout,
		host,
		config.GetMaxRetriableErrorCount(),
	)
}
