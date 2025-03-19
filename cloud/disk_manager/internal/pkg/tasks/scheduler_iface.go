package tasks

import (
	"context"
	"time"

	"github.com/golang/protobuf/proto"

	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/operation"
)

////////////////////////////////////////////////////////////////////////////////

type Scheduler interface {
	// Returns id of the task.
	ScheduleTask(
		ctx context.Context,
		taskType string,
		description string,
		request proto.Message,
		cloudID string,
		folderID string,
	) (string, error)

	// The task will be executed by a worker in certain zone.
	// Returns id of the task.
	ScheduleZonalTask(
		ctx context.Context,
		taskType string,
		description string,
		zoneID string,
		request proto.Message,
		cloudID string,
		folderID string,
	) (string, error)

	ScheduleRegularTasks(
		ctx context.Context,
		taskType string,
		description string,
		scheduleInterval time.Duration,
		maxTasksInflight int,
	)

	// Marks task as cancelled.
	// Returns true if it's already cancelling (or cancelled).
	// Returns false if it has successfully finished.
	CancelTask(ctx context.Context, taskID string) (bool, error)

	// Interrupts execution if waited task has not result yet.
	// Execution will be resumed when result is ready.
	// Returns task response.
	// Returns error if task is cancelling.
	WaitTaskAsync(
		ctx context.Context,
		execCtx ExecutionContext,
		taskID string,
	) (proto.Message, error)

	// Waits until task is finished successfully or cancelled.
	WaitTaskEnded(ctx context.Context, taskID string) error

	GetTaskMetadata(ctx context.Context, taskID string) (proto.Message, error)

	// TODO: Does it belong here?
	GetOperationProto(
		ctx context.Context,
		taskID string,
	) (*operation.Operation, error)

	// Used in tests.
	WaitTaskResponse(
		ctx context.Context,
		taskID string,
		timeout time.Duration,
	) (proto.Message, error)

	// Used in tests.
	WaitTaskCancelled(
		ctx context.Context,
		taskID string,
		timeout time.Duration,
	) error

	// Schedules no-op task. Used for testing.
	ScheduleBlankTask(ctx context.Context) (string, error)
}
