package tasks

import (
	"context"
	"testing"
	"time"

	"github.com/golang/protobuf/ptypes"
	"github.com/golang/protobuf/ptypes/empty"
	"github.com/golang/protobuf/ptypes/timestamp"
	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/mock"
	"github.com/stretchr/testify/require"
	grpc_codes "google.golang.org/grpc/codes"
	grpc_status "google.golang.org/grpc/status"

	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/operation"
	tasks_config "a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/config"
	tasks_storage "a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/storage"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/storage/mocks"
	"a.yandex-team.ru/library/go/test/assertpb"
)

////////////////////////////////////////////////////////////////////////////////

func makeDefaultConfig() *tasks_config.TasksConfig {
	pollForTaskUpdatesPeriod := "100ms"
	taskWaitingTimeout := "500ms"
	scheduleRegularTasksPeriodMin := "100ms"
	scheduleRegularTasksPeriodMax := "200ms"
	endedTaskExpirationTimeout := "1s"
	clearEndedTasksTaskScheduleInterval := "1s"

	return &tasks_config.TasksConfig{
		PollForTaskUpdatesPeriod:            &pollForTaskUpdatesPeriod,
		TaskWaitingTimeout:                  &taskWaitingTimeout,
		ScheduleRegularTasksPeriodMin:       &scheduleRegularTasksPeriodMin,
		ScheduleRegularTasksPeriodMax:       &scheduleRegularTasksPeriodMax,
		EndedTaskExpirationTimeout:          &endedTaskExpirationTimeout,
		ClearEndedTasksTaskScheduleInterval: &clearEndedTasksTaskScheduleInterval,
	}
}

////////////////////////////////////////////////////////////////////////////////

// It's impossible to use ExecutionContextMock from mocks/, because of circular
// package dependency.
type executionContextMock struct {
	mock.Mock
}

func (c *executionContextMock) Checkpoint(ctx context.Context) error {
	args := c.Called(ctx)
	return args.Error(0)
}

func (c *executionContextMock) GetOperationID(ctx context.Context) string {
	args := c.Called(ctx)
	return args.String(0)
}

func (c *executionContextMock) AddTaskDependency(
	ctx context.Context,
	taskID string,
) error {

	args := c.Called(ctx, taskID)
	return args.Error(0)
}

func createExecutionContextMock() *executionContextMock {
	execCtx := &executionContextMock{}
	execCtx.On("GetOperationID", mock.Anything).Return("toplevel_task_id")
	return execCtx
}

////////////////////////////////////////////////////////////////////////////////

func TestSchedulerScheduleTask(t *testing.T) {
	ctx := createContext()
	storage := mocks.CreateStorageMock()
	registry := CreateRegistry()
	scheduler, err := CreateScheduler(
		ctx,
		registry,
		storage,
		makeDefaultConfig(),
	)
	require.NoError(t, err)

	task := CreateTaskMock()
	_ = registry.Register(ctx, "task", func() Task { return task })

	task.On("Init", ctx, &empty.Empty{}).Return(nil)
	task.On("Save", ctx).Return([]byte{1, 2, 3}, nil)
	storage.On("CreateTask", ctx, mock.MatchedBy(func(state tasks_storage.TaskState) bool {
		ok := true
		ok = assert.Zero(t, state.ID) && ok
		ok = assert.Equal(t, "task", state.TaskType) && ok
		ok = assert.Equal(t, "Some task", state.Description) && ok
		ok = assert.EqualValues(t, 0, state.GenerationID) && ok
		ok = assert.Equal(t, tasks_storage.TaskStatusReadyToRun, state.Status) && ok
		ok = assert.Equal(t, grpc_codes.OK, state.ErrorCode) && ok
		ok = assert.Equal(t, "", state.ErrorMessage) && ok
		ok = assert.Equal(t, []byte{1, 2, 3}, state.State) && ok
		ok = assert.Equal(t, "", state.ZoneID) && ok
		ok = assert.Equal(t, "cloud", state.CloudID) && ok
		ok = assert.Equal(t, "folder", state.FolderID) && ok
		return ok
	})).Return("taskID", nil)

	taskID, err := scheduler.ScheduleTask(
		ctx,
		"task",
		"Some task",
		&empty.Empty{},
		"cloud",
		"folder",
	)
	mock.AssertExpectationsForObjects(t, task, storage)
	assert.NoError(t, err)
	assert.Equal(t, "taskID", taskID)
}

func TestSchedulerScheduleZonalTask(t *testing.T) {
	ctx := createContext()
	storage := mocks.CreateStorageMock()
	registry := CreateRegistry()
	scheduler, err := CreateScheduler(
		ctx,
		registry,
		storage,
		makeDefaultConfig(),
	)
	require.NoError(t, err)

	task := CreateTaskMock()
	_ = registry.Register(ctx, "task", func() Task { return task })

	task.On("Init", ctx, &empty.Empty{}).Return(nil)
	task.On("Save", ctx).Return([]byte{1, 2, 3}, nil)
	storage.On("CreateTask", ctx, mock.MatchedBy(func(state tasks_storage.TaskState) bool {
		ok := true
		ok = assert.Zero(t, state.ID) && ok
		ok = assert.Equal(t, "task", state.TaskType) && ok
		ok = assert.Equal(t, "Some task", state.Description) && ok
		ok = assert.EqualValues(t, 0, state.GenerationID) && ok
		ok = assert.Equal(t, tasks_storage.TaskStatusReadyToRun, state.Status) && ok
		ok = assert.Equal(t, grpc_codes.OK, state.ErrorCode) && ok
		ok = assert.Equal(t, "", state.ErrorMessage) && ok
		ok = assert.Equal(t, []byte{1, 2, 3}, state.State) && ok
		ok = assert.Equal(t, "zone", state.ZoneID) && ok
		ok = assert.Equal(t, "cloud", state.CloudID) && ok
		ok = assert.Equal(t, "folder", state.FolderID) && ok
		return ok
	})).Return("taskID", nil)

	taskID, err := scheduler.ScheduleZonalTask(
		ctx,
		"task",
		"Some task",
		"zone",
		&empty.Empty{},
		"cloud",
		"folder",
	)
	mock.AssertExpectationsForObjects(t, task, storage)
	assert.NoError(t, err)
	assert.Equal(t, "taskID", taskID)
}

func TestSchedulerScheduleTaskFailOnCreateTask(t *testing.T) {
	ctx := createContext()
	storage := mocks.CreateStorageMock()
	registry := CreateRegistry()
	scheduler, err := CreateScheduler(
		ctx,
		registry,
		storage,
		makeDefaultConfig(),
	)
	require.NoError(t, err)

	task := CreateTaskMock()
	_ = registry.Register(ctx, "task", func() Task { return task })

	task.On("Init", mock.Anything, mock.Anything).Return(nil)
	task.On("Save", mock.Anything).Return([]byte{1, 2, 3}, nil)
	storage.
		On("CreateTask", mock.Anything, mock.Anything).
		Return("", assert.AnError)

	taskID, err := scheduler.ScheduleTask(
		ctx,
		"task",
		"Some task",
		&empty.Empty{},
		"",
		"",
	)
	mock.AssertExpectationsForObjects(t, task, storage)
	assert.Error(t, err)
	assert.Zero(t, taskID)
}

func TestSchedulerCancelTask(t *testing.T) {
	ctx := createContext()
	storage := mocks.CreateStorageMock()
	registry := CreateRegistry()
	scheduler, err := CreateScheduler(
		ctx,
		registry,
		storage,
		makeDefaultConfig(),
	)
	require.NoError(t, err)

	storage.On("MarkForCancellation", ctx, "taskID", mock.Anything).Return(true, nil)

	cancelling, err := scheduler.CancelTask(ctx, "taskID")
	mock.AssertExpectationsForObjects(t, storage)
	assert.NoError(t, err)
	assert.True(t, cancelling)
}

func TestSchedulerGetTaskMetadata(t *testing.T) {
	ctx := createContext()
	storage := mocks.CreateStorageMock()
	registry := CreateRegistry()
	scheduler, err := CreateScheduler(
		ctx,
		registry,
		storage,
		makeDefaultConfig(),
	)
	require.NoError(t, err)

	task := CreateTaskMock()
	_ = registry.Register(ctx, "task", func() Task { return task })

	storage.On("GetTask", ctx, "taskID").Return(tasks_storage.TaskState{
		TaskType: "task",
		State:    []byte{1, 2, 3},
	}, nil)
	task.On("Load", ctx, []byte{1, 2, 3}).Return(nil)
	task.On("GetMetadata", ctx).Return(&empty.Empty{}, nil)

	metadata, err := scheduler.GetTaskMetadata(ctx, "taskID")
	mock.AssertExpectationsForObjects(t, task, storage)
	assert.NoError(t, err)
	assertpb.Equal(t, metadata, &empty.Empty{})
}

func TestSchedulerGetOperationProtoReadyToRun(t *testing.T) {
	ctx := createContext()
	storage := mocks.CreateStorageMock()
	registry := CreateRegistry()
	scheduler, err := CreateScheduler(
		ctx,
		registry,
		storage,
		makeDefaultConfig(),
	)
	require.NoError(t, err)

	task := CreateTaskMock()
	_ = registry.Register(ctx, "task", func() Task { return task })

	storage.On("GetTask", ctx, "taskID").Return(tasks_storage.TaskState{
		TaskType:    "task",
		State:       []byte{1, 2, 3},
		CreatedAt:   time.Unix(1, 2),
		ModifiedAt:  time.Unix(3, 4),
		ID:          "taskID",
		Description: "Some task",
		CreatedBy:   "Some UID",
		Status:      tasks_storage.TaskStatusReadyToRun,
	}, nil)
	task.On("Load", ctx, []byte{1, 2, 3}).Return(nil)
	task.On("GetMetadata", ctx).Return(&empty.Empty{}, nil)

	expectedMetadata, _ := ptypes.MarshalAny(&empty.Empty{})

	op, err := scheduler.GetOperationProto(ctx, "taskID")
	mock.AssertExpectationsForObjects(t, task, storage)
	assert.NoError(t, err)
	assertpb.Equal(t, op, &operation.Operation{
		Id:          "taskID",
		Description: "Some task",
		CreatedAt: &timestamp.Timestamp{
			Seconds: 1,
			Nanos:   2,
		},
		CreatedBy: "Some UID",
		ModifiedAt: &timestamp.Timestamp{
			Seconds: 3,
			Nanos:   4,
		},
		Done:     false,
		Metadata: expectedMetadata,
	})
}

func TestSchedulerGetOperationProtoRunning(t *testing.T) {
	ctx := createContext()
	storage := mocks.CreateStorageMock()
	registry := CreateRegistry()
	scheduler, err := CreateScheduler(
		ctx,
		registry,
		storage,
		makeDefaultConfig(),
	)
	require.NoError(t, err)

	task := CreateTaskMock()
	_ = registry.Register(ctx, "task", func() Task { return task })

	storage.On("GetTask", ctx, "taskID").Return(tasks_storage.TaskState{
		TaskType:    "task",
		State:       []byte{1, 2, 3},
		CreatedAt:   time.Unix(1, 2),
		ModifiedAt:  time.Unix(3, 4),
		ID:          "taskID",
		Description: "Some task",
		CreatedBy:   "Some UID",
		Status:      tasks_storage.TaskStatusRunning,
	}, nil)
	task.On("Load", ctx, []byte{1, 2, 3}).Return(nil)
	task.On("GetMetadata", ctx).Return(&empty.Empty{}, nil)

	expectedMetadata, _ := ptypes.MarshalAny(&empty.Empty{})

	op, err := scheduler.GetOperationProto(ctx, "taskID")
	mock.AssertExpectationsForObjects(t, task, storage)
	assert.NoError(t, err)
	assertpb.Equal(t, op, &operation.Operation{
		Id:          "taskID",
		Description: "Some task",
		CreatedAt: &timestamp.Timestamp{
			Seconds: 1,
			Nanos:   2,
		},
		CreatedBy: "Some UID",
		ModifiedAt: &timestamp.Timestamp{
			Seconds: 3,
			Nanos:   4,
		},
		Done:     false,
		Metadata: expectedMetadata,
	})
}

func TestSchedulerGetOperationProtoFinished(t *testing.T) {
	ctx := createContext()
	storage := mocks.CreateStorageMock()
	registry := CreateRegistry()
	scheduler, err := CreateScheduler(
		ctx,
		registry,
		storage,
		makeDefaultConfig(),
	)
	require.NoError(t, err)

	task := CreateTaskMock()
	_ = registry.Register(ctx, "task", func() Task { return task })

	storage.On("GetTask", ctx, "taskID").Return(tasks_storage.TaskState{
		TaskType:    "task",
		State:       []byte{1, 2, 3},
		CreatedAt:   time.Unix(1, 2),
		ModifiedAt:  time.Unix(3, 4),
		ID:          "taskID",
		Description: "Some task",
		CreatedBy:   "Some UID",
		Status:      tasks_storage.TaskStatusFinished,
	}, nil)
	task.On("Load", ctx, []byte{1, 2, 3}).Return(nil)
	task.On("GetMetadata", ctx).Return(&empty.Empty{}, nil)
	task.On("GetResponse").Return(&empty.Empty{})

	expectedMetadata, _ := ptypes.MarshalAny(&empty.Empty{})
	expectedResponse, _ := ptypes.MarshalAny(&empty.Empty{})

	op, err := scheduler.GetOperationProto(ctx, "taskID")
	mock.AssertExpectationsForObjects(t, task, storage)
	assert.NoError(t, err)
	assertpb.Equal(t, op, &operation.Operation{
		Id:          "taskID",
		Description: "Some task",
		CreatedAt: &timestamp.Timestamp{
			Seconds: 1,
			Nanos:   2,
		},
		CreatedBy: "Some UID",
		ModifiedAt: &timestamp.Timestamp{
			Seconds: 3,
			Nanos:   4,
		},
		Done:     true,
		Metadata: expectedMetadata,
		Result: &operation.Operation_Response{
			Response: expectedResponse,
		},
	})
}

func TestSchedulerGetOperationProtoReadyToCancel(t *testing.T) {
	ctx := createContext()
	storage := mocks.CreateStorageMock()
	registry := CreateRegistry()
	scheduler, err := CreateScheduler(
		ctx,
		registry,
		storage,
		makeDefaultConfig(),
	)
	require.NoError(t, err)

	task := CreateTaskMock()
	_ = registry.Register(ctx, "task", func() Task { return task })

	taskError := grpc_status.New(grpc_codes.Canceled, "An error")

	storage.On("GetTask", ctx, "taskID").Return(tasks_storage.TaskState{
		TaskType:     "task",
		State:        []byte{1, 2, 3},
		CreatedAt:    time.Unix(1, 2),
		ModifiedAt:   time.Unix(3, 4),
		ID:           "taskID",
		Description:  "Some task",
		CreatedBy:    "Some UID",
		Status:       tasks_storage.TaskStatusReadyToCancel,
		ErrorCode:    taskError.Code(),
		ErrorMessage: taskError.Message(),
	}, nil)
	task.On("Load", ctx, []byte{1, 2, 3}).Return(nil)
	task.On("GetMetadata", ctx).Return(&empty.Empty{}, nil)

	expectedMetadata, _ := ptypes.MarshalAny(&empty.Empty{})

	op, err := scheduler.GetOperationProto(ctx, "taskID")
	mock.AssertExpectationsForObjects(t, task, storage)
	assert.NoError(t, err)
	assertpb.Equal(t, op, &operation.Operation{
		Id:          "taskID",
		Description: "Some task",
		CreatedAt: &timestamp.Timestamp{
			Seconds: 1,
			Nanos:   2,
		},
		CreatedBy: "Some UID",
		ModifiedAt: &timestamp.Timestamp{
			Seconds: 3,
			Nanos:   4,
		},
		Done:     true,
		Metadata: expectedMetadata,
		Result: &operation.Operation_Error{
			Error: taskError.Proto(),
		},
	})
}

func TestSchedulerGetOperationProtoCancelling(t *testing.T) {
	ctx := createContext()
	storage := mocks.CreateStorageMock()
	registry := CreateRegistry()
	scheduler, err := CreateScheduler(
		ctx,
		registry,
		storage,
		makeDefaultConfig(),
	)
	require.NoError(t, err)

	task := CreateTaskMock()
	_ = registry.Register(ctx, "task", func() Task { return task })

	taskError := grpc_status.New(grpc_codes.Canceled, "An error")

	storage.On("GetTask", ctx, "taskID").Return(tasks_storage.TaskState{
		TaskType:     "task",
		State:        []byte{1, 2, 3},
		CreatedAt:    time.Unix(1, 2),
		ModifiedAt:   time.Unix(3, 4),
		ID:           "taskID",
		Description:  "Some task",
		CreatedBy:    "Some UID",
		Status:       tasks_storage.TaskStatusCancelling,
		ErrorCode:    taskError.Code(),
		ErrorMessage: taskError.Message(),
	}, nil)
	task.On("Load", ctx, []byte{1, 2, 3}).Return(nil)
	task.On("GetMetadata", ctx).Return(&empty.Empty{}, nil)

	expectedMetadata, _ := ptypes.MarshalAny(&empty.Empty{})

	op, err := scheduler.GetOperationProto(ctx, "taskID")
	mock.AssertExpectationsForObjects(t, task, storage)
	assert.NoError(t, err)
	assertpb.Equal(t, op, &operation.Operation{
		Id:          "taskID",
		Description: "Some task",
		CreatedAt: &timestamp.Timestamp{
			Seconds: 1,
			Nanos:   2,
		},
		CreatedBy: "Some UID",
		ModifiedAt: &timestamp.Timestamp{
			Seconds: 3,
			Nanos:   4,
		},
		Done:     true,
		Metadata: expectedMetadata,
		Result: &operation.Operation_Error{
			Error: taskError.Proto(),
		},
	})
}

func TestSchedulerGetOperationProtoCancelled(t *testing.T) {
	ctx := createContext()
	storage := mocks.CreateStorageMock()
	registry := CreateRegistry()
	scheduler, err := CreateScheduler(
		ctx,
		registry,
		storage,
		makeDefaultConfig(),
	)
	require.NoError(t, err)

	task := CreateTaskMock()
	_ = registry.Register(ctx, "task", func() Task { return task })

	taskError := grpc_status.New(grpc_codes.Canceled, "An error")

	storage.On("GetTask", ctx, "taskID").Return(tasks_storage.TaskState{
		TaskType:     "task",
		State:        []byte{1, 2, 3},
		CreatedAt:    time.Unix(1, 2),
		ModifiedAt:   time.Unix(3, 4),
		ID:           "taskID",
		Description:  "Some task",
		CreatedBy:    "Some UID",
		Status:       tasks_storage.TaskStatusCancelled,
		ErrorCode:    taskError.Code(),
		ErrorMessage: taskError.Message(),
	}, nil)
	task.On("Load", ctx, []byte{1, 2, 3}).Return(nil)
	task.On("GetMetadata", ctx).Return(&empty.Empty{}, nil)

	expectedMetadata, _ := ptypes.MarshalAny(&empty.Empty{})

	op, err := scheduler.GetOperationProto(ctx, "taskID")
	mock.AssertExpectationsForObjects(t, task, storage)
	assert.NoError(t, err)
	assertpb.Equal(t, op, &operation.Operation{
		Id:          "taskID",
		Description: "Some task",
		CreatedAt: &timestamp.Timestamp{
			Seconds: 1,
			Nanos:   2,
		},
		CreatedBy: "Some UID",
		ModifiedAt: &timestamp.Timestamp{
			Seconds: 3,
			Nanos:   4,
		},
		Done:     true,
		Metadata: expectedMetadata,
		Result: &operation.Operation_Error{
			Error: taskError.Proto(),
		},
	})
}
