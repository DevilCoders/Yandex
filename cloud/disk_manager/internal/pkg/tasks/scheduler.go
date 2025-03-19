package tasks

import (
	"context"
	"fmt"
	"time"

	"github.com/golang/protobuf/proto"
	"github.com/golang/protobuf/ptypes"
	"github.com/golang/protobuf/ptypes/empty"
	grpc_codes "google.golang.org/grpc/codes"
	grpc_status "google.golang.org/grpc/status"

	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/operation"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/headers"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/logging"
	tasks_config "a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/config"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/errors"
	tasks_storage "a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/storage"
)

////////////////////////////////////////////////////////////////////////////////

type scheduler struct {
	registry                      *Registry
	storage                       tasks_storage.Storage
	pollForTaskUpdatesPeriod      time.Duration
	taskWaitingTimeout            time.Duration
	scheduleRegularTasksPeriodMin time.Duration
	scheduleRegularTasksPeriodMax time.Duration
}

func (s *scheduler) ScheduleTask(
	ctx context.Context,
	taskType string,
	description string,
	request proto.Message,
	cloudID string,
	folderID string,
) (string, error) {

	return s.ScheduleZonalTask(
		ctx,
		taskType,
		description,
		"",
		request,
		cloudID,
		folderID,
	)
}

func (s *scheduler) ScheduleZonalTask(
	ctx context.Context,
	taskType string,
	description string,
	zoneID string,
	request proto.Message,
	cloudID string,
	folderID string,
) (string, error) {

	logging.Debug(
		ctx,
		"scheduler.ScheduleTask %v",
		taskType,
	)

	task, err := s.registry.CreateTask(ctx, taskType)
	if err != nil {
		logging.Warn(
			ctx,
			"scheduler.ScheduleTask failed to create task %v err=%v",
			taskType,
			err,
		)
		return "", err
	}

	err = task.Init(ctx, request)
	if err != nil {
		logging.Warn(
			ctx,
			"scheduler.ScheduleTask failed to init task %v err=%v",
			taskType,
			err,
		)
		return "", err
	}

	state, err := task.Save(ctx)
	if err != nil {
		logging.Warn(
			ctx,
			"scheduler.ScheduleTask failed to save task %v err=%v",
			taskType,
			err,
		)
		return "", err
	}

	createdAt := time.Now()
	metadata := tasks_storage.MakeMetadata(headers.GetTracingHeaders(ctx))
	storageFolder := getStorageFolderForTasksPinning(ctx)
	idempotencyKey := headers.GetIdempotencyKey(ctx)

	taskID, err := s.storage.CreateTask(ctx, tasks_storage.TaskState{
		ID:             "",
		IdempotencyKey: idempotencyKey,
		AccountID:      headers.GetAccountID(ctx),
		TaskType:       taskType,
		Description:    description,
		StorageFolder:  storageFolder,
		CreatedAt:      createdAt,
		CreatedBy:      headers.GetAccountID(ctx),
		ModifiedAt:     createdAt,
		GenerationID:   0,
		Status:         tasks_storage.TaskStatusReadyToRun,
		State:          state,
		Metadata:       metadata,
		Dependencies:   tasks_storage.MakeStringSet(),
		ZoneID:         zoneID,
		CloudID:        cloudID,
		FolderID:       folderID,
	})
	if err != nil {
		logging.Warn(
			ctx,
			"scheduler.ScheduleTask failed to persist task %v err=%v",
			taskType,
			err,
		)
		return "", err
	}

	logging.Info(
		ctx,
		"scheduler.ScheduleTask %v with id=%v, idempotencyKey=%v, description=%v",
		taskType,
		taskID,
		idempotencyKey,
		description,
	)

	return taskID, nil
}

func (s *scheduler) ScheduleRegularTasks(
	ctx context.Context,
	taskType string,
	description string,
	scheduleInterval time.Duration,
	maxTasksInflight int,
) {

	// TODO: Don't schedule new goroutine for each regular task type.
	go func() {
		for {
			select {
			case <-ctx.Done():
				logging.Debug(ctx, "ScheduleRegularTasks %v stopped", taskType)
				return
			case <-time.After(
				getRandomDuration(
					s.scheduleRegularTasksPeriodMin,
					s.scheduleRegularTasksPeriodMax,
				)):
			}

			logging.Debug(
				ctx,
				"scheduler.ScheduleRegularTasks %v iteration",
				taskType,
			)

			task, err := s.registry.CreateTask(ctx, taskType)
			if err != nil {
				logging.Warn(
					ctx,
					"scheduler.ScheduleRegularTasks failed to create task %v, err=%v",
					taskType,
					err,
				)
				continue
			}

			err = task.Init(ctx, &empty.Empty{})
			if err != nil {
				logging.Warn(
					ctx,
					"scheduler.ScheduleRegularTasks failed to init task %v, err=%v",
					taskType,
					err,
				)
				continue
			}

			state, err := task.Save(ctx)
			if err != nil {
				logging.Warn(
					ctx,
					"scheduler.ScheduleRegularTasks failed to save task %v, err=%v",
					taskType,
					err,
				)
				continue
			}

			createdAt := time.Now()

			err = s.storage.CreateRegularTasks(ctx, tasks_storage.TaskState{
				ID:           "",
				TaskType:     taskType,
				Description:  description,
				CreatedAt:    createdAt,
				CreatedBy:    headers.GetAccountID(ctx),
				ModifiedAt:   createdAt,
				GenerationID: 0,
				Status:       tasks_storage.TaskStatusReadyToRun,
				State:        state,
				Dependencies: tasks_storage.MakeStringSet(),
			}, scheduleInterval, maxTasksInflight)
			if err != nil {
				logging.Warn(
					ctx,
					"scheduler.ScheduleRegularTasks failed to persist task %v, err=%v",
					taskType,
					err,
				)
			}
		}
	}()
}

func (s *scheduler) CancelTask(
	ctx context.Context,
	taskID string,
) (bool, error) {

	logging.Info(
		ctx,
		"scheduler.CancelTask %v",
		taskID,
	)

	cancelling, err := s.storage.MarkForCancellation(ctx, taskID, time.Now())
	if err != nil {
		logging.Info(
			ctx,
			"scheduler.CancelTask %v failed",
			taskID,
		)
		return false, err
	}

	return cancelling, nil
}

func (s *scheduler) tryGetTaskResponse(
	ctx context.Context,
	taskID string,
) (proto.Message, error) {

	taskState, err := s.storage.GetTask(ctx, taskID)
	if err != nil {
		return nil, err
	}

	if taskState.ErrorCode != grpc_codes.OK {
		e := grpc_status.Error(
			taskState.ErrorCode,
			taskState.ErrorMessage,
		)

		return nil, &errors.DetailedError{
			Err:     e,
			Details: taskState.ErrorDetails,
			Silent:  taskState.ErrorSilent,
		}
	}

	if tasks_storage.HasResult(taskState.Status) {
		task, err := s.registry.CreateTask(ctx, taskState.TaskType)
		if err != nil {
			return nil, err
		}

		err = task.Load(ctx, taskState.State)
		if err != nil {
			return nil, err
		}

		return task.GetResponse(), nil
	}

	return nil, nil
}

func (s *scheduler) WaitTaskAsync(
	ctx context.Context,
	execCtx ExecutionContext,
	taskID string,
) (proto.Message, error) {

	dependantTaskID := execCtx.GetTaskID()

	logging.Info(ctx, "scheduler.WaitTaskAsync %v by %v", taskID, dependantTaskID)

	err := execCtx.AddTaskDependency(ctx, taskID)
	if err != nil {
		errors.LogError(
			ctx,
			err,
			"scheduler.WaitTaskAsync failed to add task dependency %v",
			taskID,
		)
		return nil, err
	}

	return s.tryGetTaskResponse(ctx, taskID)
}

func (s *scheduler) WaitTaskEnded(
	ctx context.Context,
	taskID string,
) error {

	timeout := time.After(s.taskWaitingTimeout)

	for {
		state, err := s.storage.GetTask(ctx, taskID)
		if err != nil {
			logging.Info(
				ctx,
				"scheduler.WaitTaskEnded iteration failed, taskID=%v, err=%v",
				taskID,
				err,
			)
			return err
		}

		if tasks_storage.HasEnded(state.Status) {
			return nil
		}

		select {
		case <-ctx.Done():
			logging.Debug(
				ctx,
				"scheduler.WaitTaskEnded cancelled, taskID=%v",
				taskID,
			)
			return ctx.Err()
		case <-timeout:
			logging.Debug(
				ctx,
				"scheduler.WaitTaskEnded timed out, taskID=%v",
				taskID,
			)
			return &errors.RetriableError{
				Err:              fmt.Errorf("waiting task with id=%v timed out", taskID),
				IgnoreRetryLimit: true,
			}
		case <-time.After(s.pollForTaskUpdatesPeriod):
		}
	}
}

func (s *scheduler) GetTaskMetadata(
	ctx context.Context,
	taskID string,
) (proto.Message, error) {

	logging.Debug(
		ctx,
		"scheduler.GetTaskMetadata %v",
		taskID,
	)

	taskState, err := s.storage.GetTask(ctx, taskID)
	if err != nil {
		logging.Info(
			ctx,
			"scheduler.GetTaskMetadata failed to get task from storage %v err=%v",
			taskID,
			err,
		)

		return nil, err
	}

	task, err := s.registry.CreateTask(ctx, taskState.TaskType)
	if err != nil {
		logging.Warn(
			ctx,
			"scheduler.GetTaskMetadata failed to create task descriptor %v err=%v",
			taskID,
			err,
		)

		return nil, err
	}

	err = task.Load(ctx, taskState.State)
	if err != nil {
		logging.Warn(
			ctx,
			"scheduler.GetTaskMetadata failed to load task %v err=%v",
			taskID,
			err,
		)

		return nil, err
	}

	return task.GetMetadata(ctx)
}

func (s *scheduler) GetOperationProto(
	ctx context.Context,
	taskID string,
) (*operation.Operation, error) {

	logging.Debug(
		ctx,
		"scheduler.GetOperationProto %v",
		taskID,
	)

	taskState, err := s.storage.GetTask(ctx, taskID)
	if err != nil {
		logging.Warn(
			ctx,
			"scheduler.GetOperationProto failed to get task from storage %v err=%v",
			taskID,
			err,
		)

		return nil, err
	}

	task, err := s.registry.CreateTask(ctx, taskState.TaskType)
	if err != nil {
		logging.Warn(
			ctx,
			"scheduler.GetOperationProto failed to get task descriptor %v err=%v",
			taskID,
			err,
		)

		return nil, err
	}

	err = task.Load(ctx, taskState.State)
	if err != nil {
		logging.Warn(
			ctx,
			"scheduler.GetOperationProto failed to load task %v err=%v",
			taskID,
			err,
		)

		return nil, err
	}

	createdAtProto, err := ptypes.TimestampProto(taskState.CreatedAt)
	if err != nil {
		logging.Warn(
			ctx,
			"scheduler.GetOperationProto failed to convert CreatedAt %v err=%v",
			taskID,
			err,
		)

		return nil, err
	}

	modifiedAtProto, err := ptypes.TimestampProto(taskState.ModifiedAt)
	if err != nil {
		logging.Warn(
			ctx,
			"scheduler.GetOperationProto failed to convert ModifiedAt %v err=%v",
			taskID,
			err,
		)

		return nil, err
	}

	metadata, err := task.GetMetadata(ctx)
	if err != nil {
		logging.Warn(
			ctx,
			"scheduler.GetOperationProto failed to get task metadata %v err=%v",
			taskID,
			err,
		)

		return nil, err
	}

	metadataAny, err := ptypes.MarshalAny(metadata)
	if err != nil {
		logging.Warn(
			ctx,
			"scheduler.GetOperationProto failed to convert metadata %v err=%v",
			taskID,
			err,
		)

		return nil, err
	}

	op := &operation.Operation{
		Id:          taskState.ID,
		Description: taskState.Description,
		CreatedAt:   createdAtProto,
		CreatedBy:   taskState.CreatedBy,
		ModifiedAt:  modifiedAtProto,
		Done:        tasks_storage.HasResult(taskState.Status),
		Metadata:    metadataAny,
	}

	if taskState.ErrorCode != grpc_codes.OK {
		status := grpc_status.New(taskState.ErrorCode, taskState.ErrorMessage)

		if taskState.ErrorDetails != nil {
			statusWithDetails, err := status.WithDetails(taskState.ErrorDetails)
			if err == nil {
				status = statusWithDetails
			} else {
				logging.Warn(
					ctx,
					"scheduler.GetOperationProto failed to attach ErrorDetails: %v",
					err,
				)
			}
		}

		op.Result = &operation.Operation_Error{
			Error: status.Proto(),
		}
	} else if tasks_storage.HasResult(taskState.Status) {
		responseAny, err := ptypes.MarshalAny(task.GetResponse())
		if err != nil {
			logging.Warn(
				ctx,
				"scheduler.GetOperationProto failed to convert response %v err=%v",
				taskID,
				err,
			)

			return nil, err
		}

		op.Result = &operation.Operation_Response{
			Response: responseAny,
		}
	}

	return op, nil
}

func (s *scheduler) WaitTaskResponse(
	ctx context.Context,
	taskID string,
	timeout time.Duration,
) (proto.Message, error) {

	timeoutChannel := time.After(timeout)
	iteration := 0

	wait := func() error {
		select {
		case <-ctx.Done():
			logging.Info(
				ctx,
				"scheduler.WaitTaskResponse cancelled: %v",
				ctx.Err(),
			)
			return ctx.Err()
		case <-timeoutChannel:
			return &errors.NonRetriableError{
				Err: fmt.Errorf("scheduler.WaitTaskResponse timed out"),
			}
		case <-time.After(s.pollForTaskUpdatesPeriod):
		}

		iteration++

		if iteration%20 == 0 {
			logging.Debug(
				ctx,
				"scheduler.WaitTaskResponse still waiting for task with id=%v",
				taskID,
			)
		}
		return nil
	}

	for {
		response, err := s.tryGetTaskResponse(ctx, taskID)
		if err != nil {
			logging.Info(
				ctx,
				"scheduler.WaitTaskResponse iteration failed, taskID=%v, err=%v",
				taskID,
				err,
			)
			if errors.CanRetry(err) {
				err := wait()
				if err != nil {
					return nil, err
				}

				continue
			}

			return nil, err
		}

		if response != nil {
			return response, nil
		}

		err = wait()
		if err != nil {
			return nil, err
		}
	}
}

func (s *scheduler) WaitTaskCancelled(
	ctx context.Context,
	taskID string,
	timeout time.Duration,
) error {

	timeoutChannel := time.After(timeout)
	iteration := 0

	wait := func() error {
		select {
		case <-ctx.Done():
			logging.Info(
				ctx,
				"scheduler.WaitTaskCancelled cancelled: %v",
				ctx.Err(),
			)
			return ctx.Err()
		case <-timeoutChannel:
			return &errors.NonRetriableError{
				Err: fmt.Errorf("scheduler.WaitTaskCancelled timed out"),
			}
		case <-time.After(s.pollForTaskUpdatesPeriod):
		}

		iteration++

		if iteration%20 == 0 {
			logging.Debug(
				ctx,
				"scheduler.WaitTaskCancelled still waiting for task with id=%v",
				taskID,
			)
		}
		return nil
	}

	for {
		state, err := s.storage.GetTask(ctx, taskID)
		if err != nil {
			logging.Info(
				ctx,
				"scheduler.WaitTaskCancelled iteration failed, taskID=%v, err=%v",
				taskID,
				err,
			)
			if errors.CanRetry(err) {
				err := wait()
				if err != nil {
					return err
				}

				continue
			}

			return err
		}

		if state.Status == tasks_storage.TaskStatusCancelled {
			return nil
		}

		err = wait()
		if err != nil {
			return err
		}
	}
}

func (s *scheduler) ScheduleBlankTask(ctx context.Context) (string, error) {
	return s.ScheduleTask(ctx, "tasks.Blank", "", &empty.Empty{}, "", "")
}

////////////////////////////////////////////////////////////////////////////////

func CreateScheduler(
	ctx context.Context,
	registry *Registry,
	storage tasks_storage.Storage,
	config *tasks_config.TasksConfig,
) (Scheduler, error) {

	pollForTaskUpdatesPeriod, err := time.ParseDuration(
		config.GetPollForTaskUpdatesPeriod())
	if err != nil {
		return nil, err
	}

	taskWaitingTimeout, err := time.ParseDuration(config.GetTaskWaitingTimeout())
	if err != nil {
		return nil, err
	}

	scheduleRegularTasksPeriodMin, err := time.ParseDuration(config.GetScheduleRegularTasksPeriodMin())
	if err != nil {
		return nil, err
	}

	scheduleRegularTasksPeriodMax, err := time.ParseDuration(config.GetScheduleRegularTasksPeriodMax())
	if err != nil {
		return nil, err
	}

	endedTaskExpirationTimeout, err := time.ParseDuration(config.GetEndedTaskExpirationTimeout())
	if err != nil {
		return nil, err
	}

	clearEndedTasksTaskScheduleInterval, err := time.ParseDuration(
		config.GetClearEndedTasksTaskScheduleInterval(),
	)
	if err != nil {
		return nil, err
	}

	s := &scheduler{
		registry:                      registry,
		storage:                       storage,
		pollForTaskUpdatesPeriod:      pollForTaskUpdatesPeriod,
		taskWaitingTimeout:            taskWaitingTimeout,
		scheduleRegularTasksPeriodMin: scheduleRegularTasksPeriodMin,
		scheduleRegularTasksPeriodMax: scheduleRegularTasksPeriodMax,
	}

	err = registry.Register(ctx, "tasks.Blank", func() Task {
		return &blankTask{}
	})
	if err != nil {
		return nil, err
	}

	err = registry.Register(ctx, "tasks.ClearEndedTasks", func() Task {
		return &clearEndedTasksTask{
			storage:           storage,
			expirationTimeout: endedTaskExpirationTimeout,
			limit:             int(config.GetClearEndedTasksLimit()),
		}
	})
	if err != nil {
		return nil, err
	}

	if !config.GetDisableEndedTasksClearing() {
		s.ScheduleRegularTasks(
			ctx,
			"tasks.ClearEndedTasks",
			"Clear Ended Tasks",
			clearEndedTasksTaskScheduleInterval,
			1,
		)
	}

	return s, nil
}
