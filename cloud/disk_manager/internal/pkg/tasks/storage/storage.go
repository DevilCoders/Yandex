package storage

import (
	"context"
	"fmt"
	"time"

	"github.com/golang/protobuf/proto"
	ydb_types "github.com/ydb-platform/ydb-go-sdk/v3/table/types"
	grpc_codes "google.golang.org/grpc/codes"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/errors"
)

////////////////////////////////////////////////////////////////////////////////

type TaskStatus uint32

func (s *TaskStatus) UnmarshalYDB(res ydb_types.RawValue) error {
	*s = TaskStatus(res.Int64())
	return nil
}

// NOTE: These values are stored in DB, do not shuffle them around.
const (
	TaskStatusReadyToRun      TaskStatus = iota
	TaskStatusWaitingToRun    TaskStatus = iota
	TaskStatusRunning         TaskStatus = iota
	TaskStatusFinished        TaskStatus = iota
	TaskStatusReadyToCancel   TaskStatus = iota
	TaskStatusWaitingToCancel TaskStatus = iota
	TaskStatusCancelling      TaskStatus = iota
	TaskStatusCancelled       TaskStatus = iota
)

func TaskStatusToString(status TaskStatus) string {
	switch status {
	case TaskStatusReadyToRun:
		return "ready_to_run"
	case TaskStatusWaitingToRun:
		return "waiting_to_run"
	case TaskStatusRunning:
		return "running"
	case TaskStatusFinished:
		return "finished"
	case TaskStatusReadyToCancel:
		return "ready_to_cancel"
	case TaskStatusWaitingToCancel:
		return "waiting_to_cancel"
	case TaskStatusCancelling:
		return "cancelling"
	case TaskStatusCancelled:
		return "cancelled"
	}
	return fmt.Sprintf("unknown_%v", status)
}

func HasResult(taskStatus TaskStatus) bool {
	return taskStatus > TaskStatusRunning
}

func HasEnded(taskStatus TaskStatus) bool {
	return taskStatus == TaskStatusFinished || taskStatus == TaskStatusCancelled
}

func IsCancelling(taskStatus TaskStatus) bool {
	return taskStatus >= TaskStatusReadyToCancel
}

////////////////////////////////////////////////////////////////////////////////

type Metadata struct {
	vals map[string]string
}

func (m *Metadata) Vals() map[string]string {
	return m.vals
}

func (m *Metadata) Put(key string, val string) {
	if m.vals == nil {
		m.vals = make(map[string]string)
	}

	m.vals[key] = val
}

func (m *Metadata) Remove(key string) {
	if m.vals != nil {
		delete(m.vals, key)
	}
}

func (m *Metadata) DeepCopy() Metadata {
	return MakeMetadata(m.vals)
}

func MakeMetadata(vals map[string]string) Metadata {
	if len(vals) == 0 {
		return Metadata{}
	}

	m := Metadata{
		vals: make(map[string]string),
	}

	for key, val := range vals {
		m.Put(key, val)
	}

	return m
}

////////////////////////////////////////////////////////////////////////////////

type StringSet struct {
	vals map[string]struct{}
}

func (s *StringSet) Has(val string) bool {
	if s.vals != nil {
		_, ok := s.vals[val]
		return ok
	}

	return false
}

func (s *StringSet) Vals() map[string]struct{} {
	return s.vals
}

func (s *StringSet) Add(val string) {
	if s.vals == nil {
		s.vals = make(map[string]struct{})
	}

	s.vals[val] = struct{}{}
}

func (s *StringSet) Remove(val string) {
	if s.vals != nil {
		delete(s.vals, val)
	}
}

func (s *StringSet) List() []string {
	if s.vals == nil {
		return []string{}
	}

	vals := make([]string, 0, len(s.vals))
	for v := range s.vals {
		vals = append(vals, v)
	}
	return vals
}

func (s *StringSet) DeepCopy() StringSet {
	return MakeStringSet(s.List()...)
}

func MakeStringSet(vals ...string) StringSet {
	if len(vals) == 0 {
		return StringSet{}
	}

	set := StringSet{
		vals: make(map[string]struct{}),
	}
	for _, val := range vals {
		set.Add(val)
	}
	return set
}

////////////////////////////////////////////////////////////////////////////////

// This is mapped into a DB row. If you change this struct, make sure to update
// the mapping code.
type TaskState struct {
	ID                  string
	IdempotencyKey      string
	AccountID           string
	TaskType            string
	Regular             bool
	Description         string
	StorageFolder       string
	CreatedAt           time.Time
	CreatedBy           string
	ModifiedAt          time.Time
	GenerationID        uint64
	Status              TaskStatus
	ErrorCode           grpc_codes.Code
	ErrorMessage        string
	ErrorDetails        *errors.ErrorDetails
	ErrorSilent         bool
	RetriableErrorCount uint64
	State               []byte
	Metadata            Metadata
	Dependencies        StringSet
	ChangedStateAt      time.Time
	EndedAt             time.Time
	LastHost            string
	LastRunner          string
	ZoneID              string
	CloudID             string
	FolderID            string
	EstimatedTime       time.Time

	// Internal part of the state. Fully managed by DB and can't be overwritten
	// by client.
	// TODO: Should be extracted from TaskState.
	dependants StringSet
}

func (s *TaskState) DeepCopy() TaskState {
	copied := *s

	if s.ErrorDetails != nil {
		copied.ErrorDetails = proto.Clone(s.ErrorDetails).(*errors.ErrorDetails)
	}

	copied.Metadata = s.Metadata.DeepCopy()
	copied.Dependencies = s.Dependencies.DeepCopy()
	// Unnecessary. We do it only for convenience.
	copied.dependants = s.dependants.DeepCopy()
	return copied
}

////////////////////////////////////////////////////////////////////////////////

type TaskInfo struct {
	ID           string
	GenerationID uint64
}

////////////////////////////////////////////////////////////////////////////////

type Storage interface {
	// Attempt to register new task in the storage. TaskState.ID is ignored.
	// Returns generated task id for newly created task.
	// Returns existing task id if the task with the same idempotency key and
	// the same account id already exists.
	CreateTask(ctx context.Context, state TaskState) (string, error)

	CreateRegularTasks(
		ctx context.Context,
		state TaskState,
		scheduleInterval time.Duration,
		maxTasksInflight int,
	) error

	GetTask(ctx context.Context, taskID string) (TaskState, error)

	GetTaskByIdempotencyKey(
		ctx context.Context,
		idempotencyKey string,
		accountID string,
	) (TaskState, error)

	ListTasksReadyToRun(ctx context.Context, limit uint64) ([]TaskInfo, error)

	ListTasksReadyToCancel(ctx context.Context, limit uint64) ([]TaskInfo, error)

	ListTasksRunning(ctx context.Context, limit uint64) ([]TaskInfo, error)

	ListTasksCancelling(ctx context.Context, limit uint64) ([]TaskInfo, error)

	// Lists tasks that are currently running but don't make any progress for
	// some time.
	ListTasksStallingWhileRunning(
		ctx context.Context,
		excludingHostname string,
		limit uint64,
	) ([]TaskInfo, error)

	// Lists tasks that are currently cancelling but don't make any progress for
	// some time.
	ListTasksStallingWhileCancelling(
		ctx context.Context,
		excludingHostname string,
		limit uint64,
	) ([]TaskInfo, error)

	// Fails with WrongGenerationError, if generationID does not match.
	LockTaskToRun(
		ctx context.Context,
		taskInfo TaskInfo,
		at time.Time,
		hostname string,
		runner string,
	) (TaskState, error)

	// Fails with WrongGenerationError, if generationID does not match.
	LockTaskToCancel(
		ctx context.Context,
		taskInfo TaskInfo,
		at time.Time,
		hostname string,
		runner string,
	) (TaskState, error)

	// Mark task for cancellation.
	// Returns true if it's already cancelling (or cancelled)
	// Returns false if it has successfully finished.
	MarkForCancellation(ctx context.Context, taskID string, at time.Time) (bool, error)

	// This fails with WrongGenerationError, if generationID does not match.
	UpdateTask(ctx context.Context, state TaskState) (TaskState, error)

	ClearEndedTasks(ctx context.Context, endedBefore time.Time, limit int) error
}
