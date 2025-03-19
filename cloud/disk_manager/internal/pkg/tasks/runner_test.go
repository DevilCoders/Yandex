package tasks

import (
	"context"
	"fmt"
	"math/rand"
	"sync"
	"testing"
	"time"

	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/mock"
	"github.com/stretchr/testify/require"
	grpc_codes "google.golang.org/grpc/codes"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/logging"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/errors"
	tasks_storage "a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/storage"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/storage/mocks"
)

////////////////////////////////////////////////////////////////////////////////

type mockCallback struct {
	mock.Mock
}

func (c *mockCallback) Run() {
	c.Called()
}

////////////////////////////////////////////////////////////////////////////////

type mockRunnerMetrics struct {
	mock.Mock
}

func (m *mockRunnerMetrics) OnExecutionStarted(state tasks_storage.TaskState) {
	m.Called(state)
}

func (m *mockRunnerMetrics) OnExecutionStopped() {
	m.Called()
}

func (m *mockRunnerMetrics) OnExecutionError(err error) {
	m.Called(err)
}

func (m *mockRunnerMetrics) OnError(err error) {
	m.Called(err)
}

////////////////////////////////////////////////////////////////////////////////

type mockRunner struct {
	mock.Mock
}

func (r *mockRunner) receiveTask(ctx context.Context) (taskHandle, error) {
	args := r.Called()
	return args.Get(0).(taskHandle), args.Error(1)
}

func (r *mockRunner) lockTask(
	ctx context.Context,
	taskInfo tasks_storage.TaskInfo,
) (tasks_storage.TaskState, error) {

	args := r.Called(ctx, taskInfo)
	return args.Get(0).(tasks_storage.TaskState), args.Error(1)
}

func (r *mockRunner) executeTask(
	ctx context.Context,
	execCtx *executionContext,
	task Task,
) {

	r.Called(ctx, execCtx, task)
}

func (r *mockRunner) tryExecutingTask(
	ctx context.Context,
	taskInfo tasks_storage.TaskInfo,
) error {

	args := r.Called(ctx, taskInfo)
	return args.Error(0)
}

////////////////////////////////////////////////////////////////////////////////

func createContext() context.Context {
	return logging.SetLogger(
		context.Background(),
		logging.CreateStderrLogger(logging.DebugLevel),
	)
}

////////////////////////////////////////////////////////////////////////////////

func taskMatcher(
	t *testing.T,
	expected tasks_storage.TaskState,
) func(tasks_storage.TaskState) bool {

	modifiedAtLowBound := time.Now()
	return func(actual tasks_storage.TaskState) bool {
		ok := true
		modifiedAtHighBound := time.Now()
		duration := modifiedAtHighBound.Sub(modifiedAtLowBound) / 2
		pivot := modifiedAtLowBound.Add(duration)
		ok = assert.WithinDuration(t, pivot, time.Time(actual.ModifiedAt), duration) && ok

		var zeroTime time.Time
		actual.ModifiedAt = zeroTime
		ok = assert.Equal(t, expected, actual) && ok
		return ok
	}
}

////////////////////////////////////////////////////////////////////////////////

func TestExecutionContextSaveState(t *testing.T) {
	ctx := createContext()
	storage := mocks.CreateStorageMock()
	task := CreateTaskMock()

	execCtx := makeExecutionContext(task, storage, tasks_storage.TaskState{
		ID: "taskId",
	})

	state := tasks_storage.TaskState{
		ID:    "taskId",
		State: []byte{1, 2, 3},
	}
	task.On("Save", ctx).Return(state.State, nil)
	storage.On("UpdateTask", ctx, mock.MatchedBy(taskMatcher(t, state))).Return(state, nil)

	err := execCtx.SaveState(ctx)
	mock.AssertExpectationsForObjects(t, task, storage)
	require.NoError(t, err)
}

func TestExecutionContextSaveStateFailOnError(t *testing.T) {
	ctx := createContext()
	storage := mocks.CreateStorageMock()
	task := CreateTaskMock()

	execCtx := makeExecutionContext(task, storage, tasks_storage.TaskState{
		ID: "taskId",
	})

	state := tasks_storage.TaskState{
		ID:    "taskId",
		State: []byte{1, 2, 3},
	}
	task.On("Save", ctx).Return(state.State, nil)
	storage.On("UpdateTask", ctx, mock.MatchedBy(taskMatcher(t, state))).Return(state, assert.AnError)

	err := execCtx.SaveState(ctx)
	mock.AssertExpectationsForObjects(t, task, storage)
	require.Equal(t, assert.AnError, err)
}

func TestExecutionContextGetTaskType(t *testing.T) {
	storage := mocks.CreateStorageMock()
	task := CreateTaskMock()

	execCtx := makeExecutionContext(task, storage, tasks_storage.TaskState{
		ID:       "taskId",
		TaskType: "taskType",
	})

	require.Equal(t, "taskType", execCtx.GetTaskType())
}

func TestExecutionContextGetTaskID(t *testing.T) {
	storage := mocks.CreateStorageMock()
	task := CreateTaskMock()

	execCtx := makeExecutionContext(task, storage, tasks_storage.TaskState{
		ID: "taskId",
	})

	require.Equal(t, "taskId", execCtx.GetTaskID())
}

func TestExecutionContextAddTaskDependency(t *testing.T) {
	ctx := createContext()
	storage := mocks.CreateStorageMock()
	task := CreateTaskMock()

	execCtx := makeExecutionContext(task, storage, tasks_storage.TaskState{
		ID:           "taskId1",
		Dependencies: tasks_storage.MakeStringSet(),
	})

	state := tasks_storage.TaskState{
		ID:           "taskId1",
		Dependencies: tasks_storage.MakeStringSet("taskId2"),
	}
	task.On("Save", ctx).Return(state.State, nil)
	storage.On("UpdateTask", ctx, mock.MatchedBy(taskMatcher(t, state))).Return(state, nil)

	err := execCtx.AddTaskDependency(ctx, "taskId2")
	mock.AssertExpectationsForObjects(t, task, storage)
	require.NoError(t, err)
}

func TestExecutionContextAddAnotherTaskDependency(t *testing.T) {
	ctx := createContext()
	storage := mocks.CreateStorageMock()
	task := CreateTaskMock()

	execCtx := makeExecutionContext(task, storage, tasks_storage.TaskState{
		ID:           "taskId1",
		Dependencies: tasks_storage.MakeStringSet("taskId2"),
	})

	state := tasks_storage.TaskState{
		ID:           "taskId1",
		Dependencies: tasks_storage.MakeStringSet("taskId2", "taskId3"),
	}
	task.On("Save", ctx).Return(state.State, nil)
	storage.On("UpdateTask", ctx, mock.MatchedBy(taskMatcher(t, state))).Return(state, nil)

	err := execCtx.AddTaskDependency(ctx, "taskId3")
	mock.AssertExpectationsForObjects(t, task, storage)
	require.NoError(t, err)
}

func TestRunnerForRun(t *testing.T) {
	ctx := createContext()
	storage := mocks.CreateStorageMock()
	task := CreateTaskMock()

	execCtx := makeExecutionContext(task, storage, tasks_storage.TaskState{
		ID:     "taskId",
		Status: tasks_storage.TaskStatusReadyToRun,
	})

	state := tasks_storage.TaskState{
		ID:     "taskId",
		Status: tasks_storage.TaskStatusFinished,
		State:  []byte{1, 2, 3},
	}
	task.On("Save", ctx).Return(state.State, nil)
	task.On("Run", ctx, execCtx).Return(nil)
	storage.On("UpdateTask", ctx, mock.MatchedBy(taskMatcher(t, state))).Return(state, nil)

	runnerMetrics := &mockRunnerMetrics{}

	runner := runnerForRun{metrics: runnerMetrics}
	runner.executeTask(ctx, execCtx, task)
	mock.AssertExpectationsForObjects(t, task, storage, runnerMetrics)
}

func TestRunnerForRunCtxCancelled(t *testing.T) {
	ctx, cancelCtx := context.WithCancel(createContext())
	storage := mocks.CreateStorageMock()
	task := CreateTaskMock()

	execCtx := makeExecutionContext(task, storage, tasks_storage.TaskState{
		ID:     "taskId",
		Status: tasks_storage.TaskStatusReadyToRun,
	})

	task.On("Run", ctx, execCtx).Run(func(args mock.Arguments) {
		cancelCtx()
	}).Return(assert.AnError)

	runnerMetrics := &mockRunnerMetrics{}

	runner := runnerForRun{metrics: runnerMetrics}
	runner.executeTask(ctx, execCtx, task)
	mock.AssertExpectationsForObjects(t, task, storage, runnerMetrics)
}

func TestRunnerForRunGotRetriableError(t *testing.T) {
	ctx := createContext()
	storage := mocks.CreateStorageMock()
	task := CreateTaskMock()

	execCtx := makeExecutionContext(task, storage, tasks_storage.TaskState{
		ID:                  "taskId",
		Status:              tasks_storage.TaskStatusRunning,
		RetriableErrorCount: 0,
	})

	err := &errors.RetriableError{Err: assert.AnError}

	task.On("Run", ctx, execCtx).Run(func(args mock.Arguments) {
	}).Return(err)

	state := tasks_storage.TaskState{
		ID:                  "taskId",
		Status:              tasks_storage.TaskStatusRunning,
		RetriableErrorCount: 1,
	}
	task.On("Save", ctx).Return(state.State, nil)
	storage.On("UpdateTask", ctx, mock.MatchedBy(taskMatcher(t, state))).Return(state, nil)

	runnerMetrics := &mockRunnerMetrics{}
	runnerMetrics.On("OnExecutionError", err).Return().Once()

	runner := runnerForRun{
		metrics:                runnerMetrics,
		maxRetriableErrorCount: 1,
	}
	runner.executeTask(ctx, execCtx, task)
	mock.AssertExpectationsForObjects(t, task, storage, runnerMetrics)
}

func TestRunnerForRunRetriableErrorCountExceeded(t *testing.T) {
	ctx := createContext()
	storage := mocks.CreateStorageMock()
	task := CreateTaskMock()

	execCtx := makeExecutionContext(task, storage, tasks_storage.TaskState{
		ID:                  "taskId",
		Status:              tasks_storage.TaskStatusRunning,
		RetriableErrorCount: 1,
	})

	err := &errors.RetriableError{Err: assert.AnError}

	task.On("Run", ctx, execCtx).Run(func(args mock.Arguments) {
	}).Return(err)

	state := tasks_storage.TaskState{
		ID:                  "taskId",
		Status:              tasks_storage.TaskStatusReadyToCancel,
		ErrorCode:           grpc_codes.Unknown,
		ErrorMessage:        err.Error(),
		RetriableErrorCount: 1,
	}
	task.On("Save", ctx).Return(state.State, nil)
	storage.On("UpdateTask", ctx, mock.MatchedBy(taskMatcher(t, state))).Return(state, nil)

	runnerMetrics := &mockRunnerMetrics{}
	runnerMetrics.On("OnExecutionError", err).Return().Once()
	runnerMetrics.On("OnExecutionError", &errors.NonRetriableError{Err: err}).Return().Once()

	runner := runnerForRun{
		metrics:                runnerMetrics,
		maxRetriableErrorCount: 1,
	}
	runner.executeTask(ctx, execCtx, task)
	mock.AssertExpectationsForObjects(t, task, storage, runnerMetrics)
}

func TestRunnerForRunIgnoreRetryLimit(t *testing.T) {
	ctx := createContext()
	storage := mocks.CreateStorageMock()
	task := CreateTaskMock()

	execCtx := makeExecutionContext(task, storage, tasks_storage.TaskState{
		ID:                  "taskId",
		Status:              tasks_storage.TaskStatusRunning,
		RetriableErrorCount: 1,
	})

	err := &errors.RetriableError{
		Err:              assert.AnError,
		IgnoreRetryLimit: true,
	}

	task.On("Run", ctx, execCtx).Run(func(args mock.Arguments) {
	}).Return(err)

	state := tasks_storage.TaskState{
		ID:                  "taskId",
		Status:              tasks_storage.TaskStatusRunning,
		RetriableErrorCount: 2,
	}
	task.On("Save", ctx).Return(state.State, nil)
	storage.On("UpdateTask", ctx, mock.MatchedBy(taskMatcher(t, state))).Return(state, nil)

	runnerMetrics := &mockRunnerMetrics{}
	runnerMetrics.On("OnExecutionError", err).Return().Once()

	runner := runnerForRun{
		metrics:                runnerMetrics,
		maxRetriableErrorCount: 1,
	}
	runner.executeTask(ctx, execCtx, task)
	mock.AssertExpectationsForObjects(t, task, storage, runnerMetrics)
}

func TestRunnerForRunGotNonRetriableError1(t *testing.T) {
	ctx := createContext()
	storage := mocks.CreateStorageMock()
	task := CreateTaskMock()

	execCtx := makeExecutionContext(task, storage, tasks_storage.TaskState{
		ID:     "taskId",
		Status: tasks_storage.TaskStatusReadyToRun,
	})

	// Non retriable error beats retriable error.
	failure := &errors.NonRetriableError{Err: &errors.RetriableError{Err: assert.AnError}}
	task.On("Run", ctx, execCtx).Return(failure)

	state := tasks_storage.TaskState{
		ID:           "taskId",
		Status:       tasks_storage.TaskStatusReadyToCancel,
		ErrorCode:    grpc_codes.Unknown,
		ErrorMessage: failure.Error(),
	}
	task.On("Save", ctx).Return(state.State, nil)
	storage.On("UpdateTask", ctx, mock.MatchedBy(taskMatcher(t, state))).Return(state, nil)

	runnerMetrics := &mockRunnerMetrics{}
	runnerMetrics.On("OnExecutionError", failure).Return().Once()

	runner := runnerForRun{metrics: runnerMetrics}
	runner.executeTask(ctx, execCtx, task)
	mock.AssertExpectationsForObjects(t, task, storage, runnerMetrics)
}

func TestRunnerForRunGotNonRetriableError2(t *testing.T) {
	ctx := createContext()
	storage := mocks.CreateStorageMock()
	task := CreateTaskMock()

	execCtx := makeExecutionContext(task, storage, tasks_storage.TaskState{
		ID:     "taskId",
		Status: tasks_storage.TaskStatusReadyToRun,
	})

	// Non retriable error beats retriable error.
	failure := &errors.RetriableError{Err: &errors.NonRetriableError{Err: assert.AnError}}
	task.On("Run", ctx, execCtx).Return(failure)

	state := tasks_storage.TaskState{
		ID:           "taskId",
		Status:       tasks_storage.TaskStatusReadyToCancel,
		ErrorCode:    grpc_codes.Unknown,
		ErrorMessage: failure.Error(),
	}
	task.On("Save", ctx).Return(state.State, nil)
	storage.On("UpdateTask", ctx, mock.MatchedBy(taskMatcher(t, state))).Return(state, nil)

	runnerMetrics := &mockRunnerMetrics{}
	runnerMetrics.On("OnExecutionError", failure).Return().Once()

	runner := runnerForRun{metrics: runnerMetrics}
	runner.executeTask(ctx, execCtx, task)
	mock.AssertExpectationsForObjects(t, task, storage, runnerMetrics)
}

func TestRunnerForRunGotNonCancellableError(t *testing.T) {
	ctx := createContext()
	storage := mocks.CreateStorageMock()
	task := CreateTaskMock()

	execCtx := makeExecutionContext(task, storage, tasks_storage.TaskState{
		ID:     "taskId",
		Status: tasks_storage.TaskStatusReadyToRun,
	})

	failure := &errors.RetriableError{Err: &errors.NonCancellableError{Err: assert.AnError}}
	task.On("Run", ctx, execCtx).Return(failure)

	state := tasks_storage.TaskState{
		ID:           "taskId",
		Status:       tasks_storage.TaskStatusCancelled,
		ErrorCode:    grpc_codes.Unknown,
		ErrorMessage: failure.Error(),
	}
	task.On("Save", ctx).Return(state.State, nil)
	storage.On("UpdateTask", ctx, mock.MatchedBy(taskMatcher(t, state))).Return(state, nil)

	runnerMetrics := &mockRunnerMetrics{}
	runnerMetrics.On("OnExecutionError", failure).Return().Once()

	runner := runnerForRun{metrics: runnerMetrics}
	runner.executeTask(ctx, execCtx, task)
	mock.AssertExpectationsForObjects(t, task, storage, runnerMetrics)
}

func TestRunnerForRunWrongGeneration(t *testing.T) {
	ctx := createContext()
	storage := mocks.CreateStorageMock()
	task := CreateTaskMock()

	execCtx := makeExecutionContext(task, storage, tasks_storage.TaskState{
		ID:     "taskId",
		Status: tasks_storage.TaskStatusReadyToRun,
	})

	err := &errors.WrongGenerationError{}

	task.On("Run", ctx, execCtx).Return(err)

	runnerMetrics := &mockRunnerMetrics{}
	runnerMetrics.On("OnExecutionError", err).Return().Once()

	runner := runnerForRun{metrics: runnerMetrics}
	runner.executeTask(ctx, execCtx, task)
	mock.AssertExpectationsForObjects(t, task, storage, runnerMetrics)
}

func TestRunnerForRunWrongGenerationWrappedIntoRetriableError(t *testing.T) {
	ctx := createContext()
	storage := mocks.CreateStorageMock()
	task := CreateTaskMock()

	execCtx := makeExecutionContext(task, storage, tasks_storage.TaskState{
		ID:                  "taskId",
		Status:              tasks_storage.TaskStatusReadyToRun,
		RetriableErrorCount: 0,
	})

	err := &errors.RetriableError{Err: &errors.WrongGenerationError{}}

	task.On("Run", ctx, execCtx).Return(err)

	runnerMetrics := &mockRunnerMetrics{}
	runnerMetrics.On("OnExecutionError", err).Return().Once()

	runner := runnerForRun{
		metrics:                runnerMetrics,
		maxRetriableErrorCount: 0,
	}
	runner.executeTask(ctx, execCtx, task)
	mock.AssertExpectationsForObjects(t, task, storage, runnerMetrics)
}

func TestRunnerForRunInterruptExecution(t *testing.T) {
	ctx := createContext()
	storage := mocks.CreateStorageMock()
	task := CreateTaskMock()

	execCtx := makeExecutionContext(task, storage, tasks_storage.TaskState{
		ID:     "taskId",
		Status: tasks_storage.TaskStatusReadyToRun,
	})

	err := &errors.InterruptExecutionError{}

	task.On("Run", ctx, execCtx).Return(err)

	runnerMetrics := &mockRunnerMetrics{}
	runnerMetrics.On("OnExecutionError", err).Return().Once()

	runner := runnerForRun{metrics: runnerMetrics}
	runner.executeTask(ctx, execCtx, task)
	mock.AssertExpectationsForObjects(t, task, storage, runnerMetrics)
}

func TestRunnerForRunInterruptExecutionWrappedIntoRetriableError(t *testing.T) {
	ctx := createContext()
	storage := mocks.CreateStorageMock()
	task := CreateTaskMock()

	execCtx := makeExecutionContext(task, storage, tasks_storage.TaskState{
		ID:                  "taskId",
		Status:              tasks_storage.TaskStatusReadyToRun,
		RetriableErrorCount: 0,
	})

	err := &errors.RetriableError{Err: &errors.InterruptExecutionError{}}

	task.On("Run", ctx, execCtx).Return(err)

	runnerMetrics := &mockRunnerMetrics{}
	runnerMetrics.On("OnExecutionError", err).Return().Once()

	runner := runnerForRun{
		metrics:                runnerMetrics,
		maxRetriableErrorCount: 0,
	}
	runner.executeTask(ctx, execCtx, task)
	mock.AssertExpectationsForObjects(t, task, storage, runnerMetrics)
}

func TestRunnerForRunFailWithError(t *testing.T) {
	ctx := createContext()
	storage := mocks.CreateStorageMock()
	task := CreateTaskMock()

	execCtx := makeExecutionContext(task, storage, tasks_storage.TaskState{
		ID:     "taskId",
		Status: tasks_storage.TaskStatusReadyToRun,
	})

	task.On("Run", ctx, execCtx).Return(assert.AnError)
	state := tasks_storage.TaskState{
		ID:           "taskId",
		Status:       tasks_storage.TaskStatusReadyToCancel,
		ErrorCode:    grpc_codes.Unknown,
		ErrorMessage: assert.AnError.Error(),
	}
	task.On("Save", ctx).Return(state.State, nil)
	storage.On("UpdateTask", ctx, mock.MatchedBy(taskMatcher(t, state))).Return(state, nil)

	runnerMetrics := &mockRunnerMetrics{}
	runnerMetrics.On("OnExecutionError", assert.AnError).Return().Once()

	runner := runnerForRun{metrics: runnerMetrics}
	runner.executeTask(ctx, execCtx, task)
	mock.AssertExpectationsForObjects(t, task, storage, runnerMetrics)
}

func TestRunnerForCancel(t *testing.T) {
	ctx := createContext()
	storage := mocks.CreateStorageMock()
	task := CreateTaskMock()

	execCtx := makeExecutionContext(task, storage, tasks_storage.TaskState{
		ID:     "taskId",
		Status: tasks_storage.TaskStatusReadyToCancel,
	})

	state := tasks_storage.TaskState{
		ID:     "taskId",
		Status: tasks_storage.TaskStatusCancelled,
		State:  []byte{1, 2, 3},
	}
	task.On("Save", ctx).Return(state.State, nil)
	task.On("Cancel", ctx, execCtx).Return(nil)
	storage.On("UpdateTask", ctx, mock.MatchedBy(taskMatcher(t, state))).Return(state, nil)

	runnerMetrics := &mockRunnerMetrics{}

	runner := runnerForCancel{metrics: runnerMetrics}
	runner.executeTask(ctx, execCtx, task)
	mock.AssertExpectationsForObjects(t, task, storage, runnerMetrics)
}

func TestRunnerForCancelCtxCancelled(t *testing.T) {
	ctx, cancelCtx := context.WithCancel(createContext())
	storage := mocks.CreateStorageMock()
	task := CreateTaskMock()

	execCtx := makeExecutionContext(task, storage, tasks_storage.TaskState{
		ID:     "taskId",
		Status: tasks_storage.TaskStatusReadyToCancel,
	})

	task.On("Cancel", ctx, execCtx).Run(func(args mock.Arguments) {
		cancelCtx()
	}).Return(assert.AnError)

	runnerMetrics := &mockRunnerMetrics{}
	runnerMetrics.On("OnExecutionError", assert.AnError).Return().Once()

	runner := runnerForCancel{metrics: runnerMetrics}
	runner.executeTask(ctx, execCtx, task)
	mock.AssertExpectationsForObjects(t, task, storage, runnerMetrics)
}

func TestRunnerForCancelWrongGeneration(t *testing.T) {
	ctx := createContext()
	storage := mocks.CreateStorageMock()
	task := CreateTaskMock()

	execCtx := makeExecutionContext(task, storage, tasks_storage.TaskState{
		ID:     "taskId",
		Status: tasks_storage.TaskStatusReadyToCancel,
	})

	err := &errors.WrongGenerationError{}

	task.On("Cancel", ctx, execCtx).Return(err)

	runnerMetrics := &mockRunnerMetrics{}
	runnerMetrics.On("OnExecutionError", err).Return().Once()

	runner := runnerForCancel{metrics: runnerMetrics}
	runner.executeTask(ctx, execCtx, task)
	mock.AssertExpectationsForObjects(t, task, storage, runnerMetrics)
}

func TestRunnerForCancelFailWithError(t *testing.T) {
	ctx := createContext()
	storage := mocks.CreateStorageMock()
	task := CreateTaskMock()

	execCtx := makeExecutionContext(task, storage, tasks_storage.TaskState{
		ID:     "taskId",
		Status: tasks_storage.TaskStatusReadyToCancel,
	})

	task.On("Cancel", ctx, execCtx).Return(assert.AnError)

	runnerMetrics := &mockRunnerMetrics{}
	runnerMetrics.On("OnExecutionError", assert.AnError).Return().Once()

	runner := runnerForCancel{metrics: runnerMetrics}
	runner.executeTask(ctx, execCtx, task)
	mock.AssertExpectationsForObjects(t, task, storage, runnerMetrics)
}

func TestRunnerForCancelGotNonRetriableError(t *testing.T) {
	ctx := createContext()
	storage := mocks.CreateStorageMock()
	task := CreateTaskMock()

	execCtx := makeExecutionContext(task, storage, tasks_storage.TaskState{
		ID:     "taskId",
		Status: tasks_storage.TaskStatusReadyToCancel,
	})

	err := &errors.NonRetriableError{Err: assert.AnError}

	state := tasks_storage.TaskState{
		ID:     "taskId",
		Status: tasks_storage.TaskStatusCancelled,
		State:  []byte{1, 2, 3},
	}
	task.On("Save", ctx).Return(state.State, nil)
	task.On("Cancel", ctx, execCtx).Return(err)
	storage.On("UpdateTask", ctx, mock.MatchedBy(taskMatcher(t, state))).Return(state, nil)

	runnerMetrics := &mockRunnerMetrics{}
	runnerMetrics.On("OnExecutionError", err).Return().Once()

	runner := runnerForCancel{metrics: runnerMetrics}
	runner.executeTask(ctx, execCtx, task)
	mock.AssertExpectationsForObjects(t, task, storage, runnerMetrics)
}

func TestRunnerForCancelGotNonCancellableError(t *testing.T) {
	ctx := createContext()
	storage := mocks.CreateStorageMock()
	task := CreateTaskMock()

	execCtx := makeExecutionContext(task, storage, tasks_storage.TaskState{
		ID:     "taskId",
		Status: tasks_storage.TaskStatusReadyToCancel,
	})

	err := &errors.NonCancellableError{Err: assert.AnError}

	state := tasks_storage.TaskState{
		ID:     "taskId",
		Status: tasks_storage.TaskStatusCancelled,
		State:  []byte{1, 2, 3},
	}
	task.On("Save", ctx).Return(state.State, nil)
	task.On("Cancel", ctx, execCtx).Return(err)
	storage.On("UpdateTask", ctx, mock.MatchedBy(taskMatcher(t, state))).Return(state, nil)

	runnerMetrics := &mockRunnerMetrics{}
	runnerMetrics.On("OnExecutionError", err).Return().Once()

	runner := runnerForCancel{metrics: runnerMetrics}
	runner.executeTask(ctx, execCtx, task)
	mock.AssertExpectationsForObjects(t, task, storage, runnerMetrics)
}

func TestTaskPingerOnce(t *testing.T) {
	pingPeriod := 100 * time.Millisecond
	ctx, cancelCtx := context.WithCancel(createContext())
	storage := mocks.CreateStorageMock()
	task := CreateTaskMock()
	callback := &mockCallback{}

	execCtx := makeExecutionContext(task, storage, tasks_storage.TaskState{
		ID: "taskId",
	})

	state := tasks_storage.TaskState{
		ID: "taskId",
	}
	storage.On("UpdateTask", ctx, mock.MatchedBy(taskMatcher(t, state))).Return(state, nil)

	go func() {
		// Cancel runner loop on first iteration.
		// TODO: This is bad.
		<-time.After(pingPeriod / 2)
		cancelCtx()
	}()

	taskPinger(ctx, execCtx, pingPeriod, callback.Run)
	mock.AssertExpectationsForObjects(t, task, storage, callback)
}

func TestTaskPingerImmediateFailure(t *testing.T) {
	pingPeriod := 100 * time.Millisecond
	ctx, cancelCtx := context.WithCancel(createContext())
	storage := mocks.CreateStorageMock()
	task := CreateTaskMock()
	callback := &mockCallback{}

	execCtx := makeExecutionContext(task, storage, tasks_storage.TaskState{
		ID: "taskId",
	})

	state := tasks_storage.TaskState{
		ID: "taskId",
	}
	storage.On("UpdateTask", ctx, mock.MatchedBy(taskMatcher(t, state))).Return(state, assert.AnError)
	callback.On("Run")

	go func() {
		// Cancel runner loop on first iteration.
		// TODO: This is bad.
		<-time.After(pingPeriod / 2)
		cancelCtx()
	}()

	taskPinger(ctx, execCtx, pingPeriod, callback.Run)
	mock.AssertExpectationsForObjects(t, task, storage, callback)
}

func TestTaskPingerTwice(t *testing.T) {
	pingPeriod := 100 * time.Millisecond
	ctx, cancelCtx := context.WithCancel(createContext())
	storage := mocks.CreateStorageMock()
	task := CreateTaskMock()
	callback := &mockCallback{}

	execCtx := makeExecutionContext(task, storage, tasks_storage.TaskState{
		ID: "taskId",
	})

	state := tasks_storage.TaskState{
		ID: "taskId",
	}
	storage.On("UpdateTask", ctx, mock.MatchedBy(taskMatcher(t, state))).Return(state, nil).Twice()

	go func() {
		// Cancel runner loop on second iteration.
		// TODO: This is bad.
		<-time.After(pingPeriod + pingPeriod/2)
		cancelCtx()
	}()

	taskPinger(ctx, execCtx, pingPeriod, callback.Run)
	mock.AssertExpectationsForObjects(t, task, storage, callback)
}

func TestTaskPingerFailureOnSecondIteration(t *testing.T) {
	pingPeriod := 100 * time.Millisecond
	ctx, cancelCtx := context.WithCancel(createContext())
	storage := mocks.CreateStorageMock()
	task := CreateTaskMock()
	callback := &mockCallback{}

	execCtx := makeExecutionContext(task, storage, tasks_storage.TaskState{
		ID: "taskId",
	})

	state := tasks_storage.TaskState{
		ID: "taskId",
	}
	storage.On("UpdateTask", ctx, mock.MatchedBy(taskMatcher(t, state))).Return(state, nil).Once()
	storage.On("UpdateTask", ctx, mock.MatchedBy(taskMatcher(t, state))).Return(state, assert.AnError).Once()
	callback.On("Run")

	go func() {
		// Cancel runner loop on second iteration.
		// TODO: This is bad.
		<-time.After(pingPeriod + pingPeriod/2)
		cancelCtx()
	}()

	taskPinger(ctx, execCtx, pingPeriod, callback.Run)
	mock.AssertExpectationsForObjects(t, task, storage, callback)
}

func TestTaskPingerCancelledContextInUpdateTask(t *testing.T) {
	pingPeriod := 100 * time.Millisecond
	ctx, cancelCtx := context.WithCancel(createContext())
	storage := mocks.CreateStorageMock()
	task := CreateTaskMock()
	callback := &mockCallback{}

	execCtx := makeExecutionContext(task, storage, tasks_storage.TaskState{
		ID: "taskId",
	})

	state := tasks_storage.TaskState{
		ID: "taskId",
	}
	storage.On("UpdateTask", ctx, mock.MatchedBy(taskMatcher(t, state))).Run(func(args mock.Arguments) {
		cancelCtx()
	}).Return(state, context.Canceled).Once()

	taskPinger(ctx, execCtx, pingPeriod, callback.Run)
	mock.AssertExpectationsForObjects(t, task, storage, callback)
}

func TestTryExecutingTask(t *testing.T) {
	pingPeriod := 100 * time.Millisecond
	ctx := createContext()
	storage := mocks.CreateStorageMock()
	registry := CreateRegistry()
	runner := &mockRunner{}
	runnerMetrics := &mockRunnerMetrics{}

	task := CreateTaskMock()
	_ = registry.Register(ctx, "task", func() Task { return task })

	taskInfo := tasks_storage.TaskInfo{
		ID:           "taskId",
		GenerationID: 2,
	}

	state := tasks_storage.TaskState{
		ID:           "taskId",
		TaskType:     "task",
		State:        []byte{1, 2, 3},
		GenerationID: 3,
	}
	runner.On("lockTask", ctx, taskInfo).Return(state, nil)
	task.On("Load", ctx, state.State).Return(nil)
	storage.On("UpdateTask", mock.Anything, mock.MatchedBy(taskMatcher(t, state))).Return(state, nil).Maybe()

	runnerMetrics.On("OnExecutionStarted", state)
	runner.On("executeTask", mock.Anything, mock.Anything, task)
	runnerMetrics.On("OnExecutionStopped")

	err := tryExecutingTask(ctx, storage, registry, runnerMetrics, pingPeriod, runner, taskInfo)
	mock.AssertExpectationsForObjects(t, storage, runner, runnerMetrics, task)
	assert.NoError(t, err)
}

func TestTryExecutingTaskFailToPing(t *testing.T) {
	pingPeriod := 100 * time.Millisecond
	ctx := createContext()
	storage := mocks.CreateStorageMock()
	registry := CreateRegistry()
	runner := &mockRunner{}
	runnerMetrics := &mockRunnerMetrics{}

	task := CreateTaskMock()
	_ = registry.Register(ctx, "task", func() Task { return task })

	taskInfo := tasks_storage.TaskInfo{
		ID:           "taskId",
		GenerationID: 2,
	}

	state := tasks_storage.TaskState{
		ID:           "taskId",
		TaskType:     "task",
		State:        []byte{1, 2, 3},
		GenerationID: 3,
	}

	updateTaskErr := &errors.WrongGenerationError{}

	runner.On("lockTask", ctx, taskInfo).Return(state, nil)
	task.On("Load", ctx, state.State).Return(nil)
	storage.On("UpdateTask", mock.Anything, mock.MatchedBy(taskMatcher(t, state))).Return(
		state,
		updateTaskErr,
	).Once()

	runnerMetrics.On("OnExecutionStarted", state)
	runner.On("executeTask", mock.Anything, mock.Anything, task).Run(func(args mock.Arguments) {
		// Wait for pingPeriod, so that the first ping has had a chance to run.
		// TODO: This is bad.
		<-time.After(pingPeriod)
		assert.Error(t, args.Get(0).(context.Context).Err())
	})
	runnerMetrics.On("OnExecutionStopped")

	err := tryExecutingTask(ctx, storage, registry, runnerMetrics, pingPeriod, runner, taskInfo)
	mock.AssertExpectationsForObjects(t, storage, runner, runnerMetrics, task)
	require.NoError(t, err)
}

func TestRunnerLoopReceiveTaskFailure(t *testing.T) {
	ctx := createContext()
	storage := mocks.CreateStorageMock()
	registry := CreateRegistry()
	runner := &mockRunner{}
	handle := taskHandle{
		task: tasks_storage.TaskInfo{
			ID: "taskId",
		},
		onClose: func() {},
	}

	runner.On("receiveTask", mock.Anything).Return(handle, assert.AnError)
	runnerLoop(ctx, storage, registry, runner)
	mock.AssertExpectationsForObjects(t, storage, runner)
}

func TestRunnerLoopSucceeds(t *testing.T) {
	ctx, cancelCtx := context.WithCancel(createContext())
	storage := mocks.CreateStorageMock()
	registry := CreateRegistry()
	runner := &mockRunner{}
	handle := taskHandle{
		task: tasks_storage.TaskInfo{
			ID: "taskId",
		},
		onClose: func() {},
	}

	runner.On("receiveTask", mock.Anything).Return(handle, nil)
	runner.On("tryExecutingTask", ctx, handle.task).Return(nil)

	go func() {
		// Cancel runner loop on some iteration.
		// TODO: This is bad.
		<-time.After(time.Second)
		cancelCtx()
	}()
	runnerLoop(ctx, storage, registry, runner)
	mock.AssertExpectationsForObjects(t, storage, runner)
}

func testListerLoop(
	t *testing.T,
	listenersPerChannel int,
	channelCount int,
	taskCount int,
) {

	ctx, cancelCtx := context.WithCancel(createContext())
	tasks := make([]tasks_storage.TaskInfo, 0)
	for i := 0; i < taskCount; i++ {
		tasks = append(tasks, tasks_storage.TaskInfo{
			ID: fmt.Sprintf("TaskID%v", i),
		})
	}
	lister := newLister(
		ctx,
		func(ctx context.Context, limit uint64) ([]tasks_storage.TaskInfo, error) {
			return tasks, nil
		},
		uint64(channelCount),
		50*time.Millisecond,
		100*time.Millisecond,
	)

	receivedTasks := make([]tasks_storage.TaskInfo, 0)
	var receivedTasksMutex sync.Mutex

	var wg sync.WaitGroup
	listenerCount := channelCount * listenersPerChannel
	wg.Add(listenerCount)

	listenerCtx := context.Background()

	for i := 0; i < channelCount; i++ {
		channel := lister.channels[i]

		for j := 0; j < listenersPerChannel; j++ {
			go func() {
				defer wg.Done()

				handle, _ := channel.receive(listenerCtx)
				defer handle.close()

				receivedTasksMutex.Lock()
				defer receivedTasksMutex.Unlock()
				receivedTasks = append(receivedTasks, handle.task)
			}()
		}
	}
	wg.Wait()

	require.Equal(t, listenerCount, len(receivedTasks))
	for _, task := range receivedTasks {
		assert.Contains(t, tasks, task)
	}

	cancelCtx()
	// Ensure that channels are closed.
	for _, channel := range lister.channels {
		_, err := channel.receive(listenerCtx)
		assert.Error(t, err)
		assert.Contains(t, err.Error(), "closed")
	}
}

func TestListerLoop(t *testing.T) {
	testListerLoop(t, 1, 1, 2)
	testListerLoop(t, 1, 2, 1)
	testListerLoop(t, 1, 10, 3)
	testListerLoop(t, 1, 10, 21)
	testListerLoop(t, 3, 1, 2)
	testListerLoop(t, 3, 2, 1)
	testListerLoop(t, 3, 10, 3)
	testListerLoop(t, 3, 10, 21)
}

func TestListerLoopCancellingWhileReceiving(t *testing.T) {
	ctx, cancelCtx := context.WithCancel(createContext())
	defer cancelCtx()

	taskCount := 10
	channelCount := taskCount

	tasks := make([]tasks_storage.TaskInfo, 0)
	for i := 0; i < taskCount; i++ {
		tasks = append(tasks, tasks_storage.TaskInfo{
			ID: fmt.Sprintf("TaskID%v", i),
		})
	}
	lister := newLister(
		ctx,
		func(ctx context.Context, limit uint64) ([]tasks_storage.TaskInfo, error) {
			return tasks, nil
		},
		uint64(channelCount),
		50*time.Millisecond,
		100*time.Millisecond,
	)

	receivedTasks := make([]tasks_storage.TaskInfo, 0)
	var receivedTasksMutex sync.Mutex

	var wg sync.WaitGroup
	wg.Add(channelCount)

	for i := 0; i < channelCount; i++ {
		channel := lister.channels[i]
		go func() {
			defer wg.Done()

			handle, err := channel.receive(ctx)
			if err == nil {
				defer handle.close()
				receivedTasksMutex.Lock()
				defer receivedTasksMutex.Unlock()
				receivedTasks = append(receivedTasks, handle.task)
			}

			if rand.Intn(2) == 0 {
				cancelCtx()
			}
		}()
	}
	wg.Wait()

	for _, task := range receivedTasks {
		assert.Contains(t, tasks, task)
	}
}
