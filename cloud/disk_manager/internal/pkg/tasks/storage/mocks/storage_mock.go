package mocks

import (
	"context"
	"time"

	"github.com/stretchr/testify/mock"

	tasks_storage "a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/storage"
)

////////////////////////////////////////////////////////////////////////////////

type StorageMock struct {
	mock.Mock
}

func (s *StorageMock) CreateTask(
	ctx context.Context,
	state tasks_storage.TaskState,
) (string, error) {

	args := s.Called(ctx, state)
	return args.String(0), args.Error(1)
}

func (s *StorageMock) CreateRegularTasks(
	ctx context.Context,
	state tasks_storage.TaskState,
	scheduleInterval time.Duration,
	maxTasksInflight int,
) error {

	args := s.Called(ctx, state, scheduleInterval, maxTasksInflight)
	return args.Error(0)
}

func (s *StorageMock) GetTask(
	ctx context.Context,
	taskID string,
) (tasks_storage.TaskState, error) {

	args := s.Called(ctx, taskID)
	return args.Get(0).(tasks_storage.TaskState), args.Error(1)
}

func (s *StorageMock) GetTaskByIdempotencyKey(
	ctx context.Context,
	idempotencyKey string,
	accountID string,
) (tasks_storage.TaskState, error) {

	args := s.Called(ctx, idempotencyKey, accountID)
	return args.Get(0).(tasks_storage.TaskState), args.Error(1)
}

func (s *StorageMock) ListTasksReadyToRun(
	ctx context.Context,
	limit uint64,
) ([]tasks_storage.TaskInfo, error) {

	args := s.Called(ctx, limit)
	res, _ := args.Get(0).([]tasks_storage.TaskInfo)
	return res, args.Error(1)
}

func (s *StorageMock) ListTasksReadyToCancel(
	ctx context.Context,
	limit uint64,
) ([]tasks_storage.TaskInfo, error) {

	args := s.Called(ctx, limit)
	res, _ := args.Get(0).([]tasks_storage.TaskInfo)
	return res, args.Error(1)
}

func (s *StorageMock) ListTasksRunning(
	ctx context.Context,
	limit uint64,
) ([]tasks_storage.TaskInfo, error) {

	args := s.Called(ctx, limit)
	res, _ := args.Get(0).([]tasks_storage.TaskInfo)
	return res, args.Error(1)
}

func (s *StorageMock) ListTasksCancelling(
	ctx context.Context,
	limit uint64,
) ([]tasks_storage.TaskInfo, error) {

	args := s.Called(ctx, limit)
	res, _ := args.Get(0).([]tasks_storage.TaskInfo)
	return res, args.Error(1)
}

func (s *StorageMock) ListTasksStallingWhileRunning(
	ctx context.Context,
	hostname string,
	limit uint64,
) ([]tasks_storage.TaskInfo, error) {

	args := s.Called(ctx, hostname, limit)
	res, _ := args.Get(0).([]tasks_storage.TaskInfo)
	return res, args.Error(1)
}

func (s *StorageMock) ListTasksStallingWhileCancelling(
	ctx context.Context,
	hostname string,
	limit uint64,
) ([]tasks_storage.TaskInfo, error) {

	args := s.Called(ctx, hostname, limit)
	res, _ := args.Get(0).([]tasks_storage.TaskInfo)
	return res, args.Error(1)
}

func (s *StorageMock) LockTaskToRun(
	ctx context.Context,
	taskInfo tasks_storage.TaskInfo,
	at time.Time,
	hostname string,
	runner string,
) (tasks_storage.TaskState, error) {

	args := s.Called(ctx, taskInfo, at, hostname, runner)
	return args.Get(0).(tasks_storage.TaskState), args.Error(1)
}

func (s *StorageMock) LockTaskToCancel(
	ctx context.Context,
	taskInfo tasks_storage.TaskInfo,
	at time.Time,
	hostname string,
	runner string,
) (tasks_storage.TaskState, error) {

	args := s.Called(ctx, taskInfo, at, hostname, runner)
	return args.Get(0).(tasks_storage.TaskState), args.Error(1)
}

func (s *StorageMock) MarkForCancellation(
	ctx context.Context,
	taskID string,
	at time.Time,
) (bool, error) {

	args := s.Called(ctx, taskID, at)
	return args.Bool(0), args.Error(1)
}

func (s *StorageMock) UpdateTask(
	ctx context.Context,
	state tasks_storage.TaskState,
) (tasks_storage.TaskState, error) {

	args := s.Called(ctx, state)
	return args.Get(0).(tasks_storage.TaskState), args.Error(1)
}

func (s *StorageMock) ClearEndedTasks(
	ctx context.Context,
	endedBefore time.Time,
	limit int,
) error {

	args := s.Called(ctx, endedBefore, limit)
	return args.Error(0)
}

////////////////////////////////////////////////////////////////////////////////

func CreateStorageMock() *StorageMock {
	return &StorageMock{}
}

////////////////////////////////////////////////////////////////////////////////

// Ensure that StorageMock implements tasks_storage.Storage.
func assertStorageMockIsStorage(arg *StorageMock) tasks_storage.Storage {
	return arg
}
