package storage

import (
	"context"
	"time"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/logging"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/persistence"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/errors"
)

////////////////////////////////////////////////////////////////////////////////

type storageYDB struct {
	db                  *persistence.YDBClient
	folder              string
	tablesPath          string
	taskStallingTimeout time.Duration
	taskTypeWhitelist   []string
	taskTypeBlacklist   []string
	ZoneID              string
	metrics             storageMetrics
}

func (s *storageYDB) CreateTask(
	ctx context.Context,
	state TaskState,
) (string, error) {

	var taskID string

	err := s.db.Execute(
		ctx,
		func(ctx context.Context, session *persistence.Session) error {
			var err error
			taskID, err = s.createTask(ctx, session, state)
			return err
		},
	)
	if err != nil {
		logging.Warn(ctx, "CreateTask failed: %v", err)
	}

	return taskID, err
}

func (s *storageYDB) CreateRegularTasks(
	ctx context.Context,
	state TaskState,
	scheduleInterval time.Duration,
	maxTasksInflight int,
) error {

	return s.db.Execute(
		ctx,
		func(ctx context.Context, session *persistence.Session) error {
			return s.createRegularTasks(
				ctx,
				session,
				state,
				scheduleInterval,
				maxTasksInflight,
			)
		},
	)
}

func (s *storageYDB) GetTask(
	ctx context.Context,
	taskID string,
) (TaskState, error) {

	var task TaskState

	err := s.db.Execute(
		ctx,
		func(ctx context.Context, session *persistence.Session) error {
			var err error
			task, err = s.getTask(ctx, session, taskID)
			return err
		},
	)
	if err != nil {
		errors.LogError(ctx, err, "GetTask failed")
	}

	return task, err
}

func (s *storageYDB) GetTaskByIdempotencyKey(
	ctx context.Context,
	idempotencyKey string,
	accountID string,
) (TaskState, error) {

	var task TaskState

	err := s.db.Execute(
		ctx,
		func(ctx context.Context, session *persistence.Session) error {
			var err error
			task, err = s.getTaskByIdempotencyKey(
				ctx,
				session,
				idempotencyKey,
				accountID,
			)
			return err
		},
	)
	if err != nil {
		errors.LogError(ctx, err, "GetTaskByIdempotencyKey")
	}

	return task, err
}

func (s *storageYDB) ListTasksReadyToRun(
	ctx context.Context,
	limit uint64,
) ([]TaskInfo, error) {

	var tasks []TaskInfo

	err := s.db.Execute(
		ctx,
		func(ctx context.Context, session *persistence.Session) error {
			var err error
			tasks, err = s.listTasks(
				ctx,
				session,
				"ready_to_run",
				limit,
			)
			return err
		},
	)
	if err != nil {
		logging.Warn(ctx, "ListTasksReadyToRun failed: %v", err)
	}

	return tasks, err
}

func (s *storageYDB) ListTasksReadyToCancel(
	ctx context.Context,
	limit uint64,
) ([]TaskInfo, error) {

	var tasks []TaskInfo

	err := s.db.Execute(
		ctx,
		func(ctx context.Context, session *persistence.Session) error {
			var err error
			tasks, err = s.listTasks(
				ctx,
				session,
				"ready_to_cancel",
				limit,
			)
			return err
		},
	)
	if err != nil {
		logging.Warn(ctx, "ListTasksReadyToCancel failed: %v", err)
	}

	return tasks, err
}

func (s *storageYDB) ListTasksRunning(
	ctx context.Context,
	limit uint64,
) ([]TaskInfo, error) {

	var tasks []TaskInfo

	err := s.db.Execute(
		ctx,
		func(ctx context.Context, session *persistence.Session) error {
			var err error
			tasks, err = s.listTasks(
				ctx,
				session,
				"running",
				limit,
			)
			return err
		},
	)
	if err != nil {
		logging.Warn(ctx, "ListTasksRunning failed: %v", err)
	}

	return tasks, err
}

func (s *storageYDB) ListTasksCancelling(
	ctx context.Context,
	limit uint64,
) ([]TaskInfo, error) {

	var tasks []TaskInfo

	err := s.db.Execute(
		ctx,
		func(ctx context.Context, session *persistence.Session) error {
			var err error
			tasks, err = s.listTasks(
				ctx,
				session,
				"cancelling",
				limit,
			)
			return err
		},
	)
	if err != nil {
		logging.Warn(ctx, "ListTasksCancelling failed: %v", err)
	}

	return tasks, err
}

func (s *storageYDB) ListTasksStallingWhileRunning(
	ctx context.Context,
	excludingHostname string,
	limit uint64,
) ([]TaskInfo, error) {

	var tasks []TaskInfo

	err := s.db.Execute(
		ctx,
		func(ctx context.Context, session *persistence.Session) error {
			var err error
			tasks, err = s.listTasksStallingWhileExecuting(
				ctx,
				session,
				excludingHostname,
				"running",
				limit,
			)
			return err
		},
	)
	if err != nil {
		logging.Warn(ctx, "ListTasksStallingWhileRunning failed: %v", err)
	}

	return tasks, err
}

func (s *storageYDB) ListTasksStallingWhileCancelling(
	ctx context.Context,
	excludingHostname string,
	limit uint64,
) ([]TaskInfo, error) {

	var tasks []TaskInfo

	err := s.db.Execute(
		ctx,
		func(ctx context.Context, session *persistence.Session) error {
			var err error
			tasks, err = s.listTasksStallingWhileExecuting(
				ctx,
				session,
				excludingHostname,
				"cancelling",
				limit,
			)
			return err
		},
	)
	if err != nil {
		logging.Warn(ctx, "ListTasksStallingWhileCancelling failed: %v", err)
	}

	return tasks, err
}

func (s *storageYDB) LockTaskToRun(
	ctx context.Context,
	taskInfo TaskInfo,
	at time.Time,
	hostname string,
	runner string,
) (TaskState, error) {

	var task TaskState

	err := s.db.Execute(
		ctx,
		func(ctx context.Context, session *persistence.Session) error {
			var err error
			task, err = s.lockTaskToExecute(
				ctx,
				session,
				taskInfo,
				at,
				func(status TaskStatus) bool {
					return status == TaskStatusReadyToRun || status == TaskStatusRunning
				},
				TaskStatusRunning,
				hostname,
				runner,
			)
			return err
		},
	)
	if err != nil {
		errors.LogError(ctx, err, "LockTaskToRun")
	}

	return task, err
}

func (s *storageYDB) LockTaskToCancel(
	ctx context.Context,
	taskInfo TaskInfo,
	at time.Time,
	hostname string,
	runner string,
) (TaskState, error) {

	var task TaskState

	err := s.db.Execute(
		ctx,
		func(ctx context.Context, session *persistence.Session) error {
			var err error
			task, err = s.lockTaskToExecute(
				ctx,
				session,
				taskInfo,
				at,
				func(status TaskStatus) bool {
					return status == TaskStatusReadyToCancel || status == TaskStatusCancelling
				},
				TaskStatusCancelling,
				hostname,
				runner,
			)
			return err
		},
	)
	if err != nil {
		errors.LogError(ctx, err, "LockTaskToCancel failed")
	}

	return task, err
}

func (s *storageYDB) MarkForCancellation(
	ctx context.Context,
	taskID string,
	at time.Time,
) (bool, error) {

	var cancelling bool

	err := s.db.Execute(
		ctx,
		func(ctx context.Context, session *persistence.Session) error {
			var err error
			cancelling, err = s.markForCancellation(ctx, session, taskID, at)
			return err
		},
	)
	if err != nil {
		errors.LogError(ctx, err, "MarkForCancellation failed")
	}

	return cancelling, err
}

func (s *storageYDB) UpdateTask(
	ctx context.Context,
	state TaskState,
) (TaskState, error) {

	var newState TaskState

	err := s.db.Execute(
		ctx,
		func(ctx context.Context, session *persistence.Session) error {
			var err error
			newState, err = s.updateTask(ctx, session, state)
			return err
		},
	)
	if err != nil {
		errors.LogError(ctx, err, "UpdateTask failed")
	}

	return newState, err
}

func (s *storageYDB) ClearEndedTasks(
	ctx context.Context,
	endedBefore time.Time,
	limit int,
) error {

	err := s.db.Execute(
		ctx,
		func(ctx context.Context, session *persistence.Session) error {
			return s.clearEndedTasks(ctx, session, endedBefore, limit)
		},
	)
	if err != nil {
		logging.Warn(ctx, "ClearEndedTasks failed: %v", err)
	}

	return err
}
