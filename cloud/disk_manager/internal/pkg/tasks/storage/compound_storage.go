package storage

import (
	"context"
	"fmt"
	"time"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/monitoring/metrics"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/persistence"
	tasks_config "a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/config"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/errors"
)

////////////////////////////////////////////////////////////////////////////////

type compoundStorage struct {
	legacyStorageFolder string
	legacyStorage       Storage

	storageFolder string
	storage       Storage
}

func (s *compoundStorage) invoke(
	ctx context.Context,
	call func(Storage) error,
) error {

	err := call(s.legacyStorage)
	if err != nil {
		if errors.Is(err, &errors.NotFoundError{}) {
			return call(s.storage)
		}

		return err
	}

	return nil
}

func (s *compoundStorage) visit(
	ctx context.Context,
	call func(Storage) error,
) error {

	err := call(s.legacyStorage)
	if err != nil {
		return err
	}

	return call(s.storage)
}

func (s *compoundStorage) dispatch(
	ctx context.Context,
	storageFolder string,
	call func(Storage) error,
) error {

	if storageFolder == s.legacyStorageFolder {
		return call(s.legacyStorage)
	} else if storageFolder == s.storageFolder {
		return call(s.storage)
	}

	return &errors.NonRetriableError{
		Err: fmt.Errorf(
			"storageFolder=%v should be one of %v, %v",
			storageFolder,
			s.legacyStorageFolder,
			s.storageFolder,
		),
	}
}

func (s *compoundStorage) CreateTask(
	ctx context.Context,
	state TaskState,
) (string, error) {

	if len(state.StorageFolder) == 0 {
		existing, err := s.legacyStorage.GetTaskByIdempotencyKey(
			ctx,
			state.IdempotencyKey,
			state.AccountID,
		)
		if err != nil {
			if errors.Is(err, &errors.NotFoundError{}) {
				return s.storage.CreateTask(ctx, state)
			}

			return "", err
		}

		return existing.ID, nil
	}

	var taskID string
	var err error

	err = s.dispatch(ctx, state.StorageFolder, func(storage Storage) error {
		taskID, err = storage.CreateTask(ctx, state)
		return err
	})
	return taskID, err
}

func (s *compoundStorage) CreateRegularTasks(
	ctx context.Context,
	state TaskState,
	scheduleInterval time.Duration,
	maxTasksInflight int,
) error {

	// Don't need to use legacyStorage here.
	return s.storage.CreateRegularTasks(
		ctx,
		state,
		scheduleInterval,
		maxTasksInflight,
	)
}

func (s *compoundStorage) GetTask(
	ctx context.Context,
	taskID string,
) (state TaskState, err error) {

	err = s.invoke(ctx, func(storage Storage) error {
		state, err = storage.GetTask(ctx, taskID)
		return err
	})
	return state, err
}

func (s *compoundStorage) GetTaskByIdempotencyKey(
	ctx context.Context,
	idempotencyKey string,
	accountID string,
) (state TaskState, err error) {

	err = s.invoke(ctx, func(storage Storage) error {
		state, err = storage.GetTaskByIdempotencyKey(
			ctx,
			idempotencyKey,
			accountID,
		)
		return err
	})
	return state, err
}

func (s *compoundStorage) ListTasksReadyToRun(
	ctx context.Context,
	limit uint64,
) (taskInfos []TaskInfo, err error) {

	err = s.visit(ctx, func(storage Storage) error {
		values, err := storage.ListTasksReadyToRun(ctx, limit)
		taskInfos = append(taskInfos, values...)
		return err
	})
	return taskInfos, err
}

func (s *compoundStorage) ListTasksReadyToCancel(
	ctx context.Context,
	limit uint64,
) (taskInfos []TaskInfo, err error) {

	err = s.visit(ctx, func(storage Storage) error {
		values, err := storage.ListTasksReadyToCancel(ctx, limit)
		taskInfos = append(taskInfos, values...)
		return err
	})
	return taskInfos, err
}

func (s *compoundStorage) ListTasksRunning(
	ctx context.Context,
	limit uint64,
) (taskInfos []TaskInfo, err error) {

	err = s.visit(ctx, func(storage Storage) error {
		values, err := storage.ListTasksRunning(ctx, limit)
		taskInfos = append(taskInfos, values...)
		return err
	})
	return taskInfos, err
}

func (s *compoundStorage) ListTasksCancelling(
	ctx context.Context,
	limit uint64,
) (taskInfos []TaskInfo, err error) {

	err = s.visit(ctx, func(storage Storage) error {
		values, err := storage.ListTasksCancelling(ctx, limit)
		taskInfos = append(taskInfos, values...)
		return err
	})
	return taskInfos, err
}

func (s *compoundStorage) ListTasksStallingWhileRunning(
	ctx context.Context,
	excludingHostname string,
	limit uint64,
) (taskInfos []TaskInfo, err error) {

	err = s.visit(ctx, func(storage Storage) error {
		values, err := storage.ListTasksStallingWhileRunning(
			ctx,
			excludingHostname,
			limit,
		)
		taskInfos = append(taskInfos, values...)
		return err
	})
	return taskInfos, err
}

func (s *compoundStorage) ListTasksStallingWhileCancelling(
	ctx context.Context,
	excludingHostname string,
	limit uint64,
) (taskInfos []TaskInfo, err error) {

	err = s.visit(ctx, func(storage Storage) error {
		values, err := storage.ListTasksStallingWhileCancelling(
			ctx,
			excludingHostname,
			limit,
		)
		taskInfos = append(taskInfos, values...)
		return err
	})
	return taskInfos, err
}

func (s *compoundStorage) LockTaskToRun(
	ctx context.Context,
	taskInfo TaskInfo,
	at time.Time,
	hostname string,
	runner string,
) (state TaskState, err error) {

	err = s.invoke(ctx, func(storage Storage) error {
		state, err = storage.LockTaskToRun(
			ctx,
			taskInfo,
			at,
			hostname,
			runner,
		)
		return err
	})
	return state, err
}

func (s *compoundStorage) LockTaskToCancel(
	ctx context.Context,
	taskInfo TaskInfo,
	at time.Time,
	hostname string,
	runner string,
) (state TaskState, err error) {

	err = s.invoke(ctx, func(storage Storage) error {
		state, err = storage.LockTaskToCancel(
			ctx,
			taskInfo,
			at,
			hostname,
			runner,
		)
		return err
	})
	return state, err
}

func (s *compoundStorage) MarkForCancellation(
	ctx context.Context,
	taskID string,
	at time.Time,
) (cancelling bool, err error) {

	err = s.invoke(ctx, func(storage Storage) error {
		cancelling, err = storage.MarkForCancellation(ctx, taskID, at)
		return err
	})
	return cancelling, err
}

func (s *compoundStorage) UpdateTask(
	ctx context.Context,
	state TaskState,
) (res TaskState, err error) {

	err = s.invoke(ctx, func(storage Storage) error {
		res, err = storage.UpdateTask(ctx, state)
		return err
	})
	return res, err
}

func (s *compoundStorage) ClearEndedTasks(
	ctx context.Context,
	endedBefore time.Time,
	limit int,
) error {

	// Don't need to use legacyStorage here.
	return s.storage.ClearEndedTasks(ctx, endedBefore, limit)
}

////////////////////////////////////////////////////////////////////////////////

func CreateStorage(
	config *tasks_config.TasksConfig,
	metricsRegistry metrics.Registry,
	db *persistence.YDBClient,
) (Storage, error) {

	taskStallingTimeout, err := time.ParseDuration(config.GetTaskStallingTimeout())
	if err != nil {
		return nil, err
	}

	storage := &storageYDB{
		db:                  db,
		folder:              config.GetStorageFolder(),
		tablesPath:          db.AbsolutePath(config.GetStorageFolder()),
		taskStallingTimeout: taskStallingTimeout,
		taskTypeWhitelist:   config.GetTaskTypeWhitelist(),
		taskTypeBlacklist:   config.GetTaskTypeBlacklist(),
		ZoneID:              config.GetZoneId(),
		metrics:             makeStorageMetrics(metricsRegistry),
	}

	if len(config.GetLegacyStorageFolder()) == 0 {
		return storage, nil
	}

	// Ignore legacy metrics.
	legacyStorage := &storageYDB{
		db:                  db,
		folder:              config.GetLegacyStorageFolder(),
		tablesPath:          db.AbsolutePath(config.GetLegacyStorageFolder()),
		taskStallingTimeout: taskStallingTimeout,
		taskTypeWhitelist:   config.GetTaskTypeWhitelist(),
		taskTypeBlacklist:   config.GetTaskTypeBlacklist(),
		ZoneID:              config.GetZoneId(),
		metrics:             makeStorageMetrics(metrics.CreateEmptyRegistry()),
	}

	return &compoundStorage{
		legacyStorageFolder: config.GetLegacyStorageFolder(),
		legacyStorage:       legacyStorage,

		storageFolder: config.GetStorageFolder(),
		storage:       storage,
	}, nil
}
