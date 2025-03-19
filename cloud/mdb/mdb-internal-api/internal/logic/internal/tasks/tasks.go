package tasks

import (
	"context"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/search"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/sessions"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/tasks"
	"a.yandex-team.ru/library/go/core/xerrors"
)

//go:generate ../../../../../scripts/mockgen.sh Tasks

type Tasks interface {
	CreateTask(ctx context.Context, session sessions.Session, args tasks.CreateTaskArgs) (operations.Operation, error)
	CreateFinishedTask(ctx context.Context, session sessions.Session, cid string, revision int64, opType operations.Type, metadata interface{}, atCurrentRev bool) (operations.Operation, error)

	CreateCluster(ctx context.Context, session sessions.Session, cid string, revision int64, taskType tasks.Type, opType operations.Type, metadata operations.Metadata, s3bucket optional.String, securityGroupIDs []string, service string, attributesExtractor search.AttributesExtractor, opts ...CreateClusterOption) (operations.Operation, error)
	ModifyCluster(ctx context.Context, session sessions.Session, cid string, revision int64, taskType tasks.Type, opType operations.Type, securityGroupIDs optional.Strings, service string, attributesExtractor search.AttributesExtractor, opts ...ModifyClusterOption) (operations.Operation, error)
	DeleteCluster(ctx context.Context, session sessions.Session, cid string, revision int64, taskTypes DeleteClusterTaskTypes, opType operations.Type, s3Buckets DeleteClusterS3Buckets, opts ...DeleteClusterOption) (operations.Operation, error)
	StartCluster(ctx context.Context, session sessions.Session, cid string, revision int64, taskType tasks.Type, opType operations.Type, metadata operations.Metadata, opts ...StartClusterOption) (operations.Operation, error)
	StopCluster(ctx context.Context, session sessions.Session, cid string, revision int64, taskType tasks.Type, opType operations.Type, metadata operations.Metadata) (operations.Operation, error)
	MoveCluster(ctx context.Context, session sessions.Session, cid string, revision int64, opType operations.Type, metadata operations.Metadata, opts ...MoveClusterOption) (operations.Operation, error)
	BackupCluster(ctx context.Context, session sessions.Session, cid string, revision int64, taskType tasks.Type, opType operations.Type, metadata operations.Metadata, opts ...BackupClusterOption) (operations.Operation, error)
	UpgradeCluster(ctx context.Context, session sessions.Session, cid string, revision int64, taskType tasks.Type, opType operations.Type, metadata operations.Metadata, opts ...UpgradeClusterOption) (operations.Operation, error)
}

type CreateClusterOptions struct {
	TaskArgs map[string]interface{}
	Timeout  optional.Duration
}

type CreateClusterOption func(options *CreateClusterOptions)

func CreateTaskArgs(a map[string]interface{}) CreateClusterOption {
	return func(options *CreateClusterOptions) {
		options.TaskArgs = a
	}
}

func CreateTaskTimeout(d time.Duration) CreateClusterOption {
	return func(options *CreateClusterOptions) {
		options.Timeout.Set(d)
	}
}

type ModifyClusterOptions struct {
	TaskArgs map[string]interface{}
	Timeout  optional.Duration
}

type ModifyClusterOption func(options *ModifyClusterOptions)

func ModifyTaskArgs(a map[string]interface{}) ModifyClusterOption {
	return func(options *ModifyClusterOptions) {
		options.TaskArgs = a
	}
}

func ModifyTimeout(d time.Duration) ModifyClusterOption {
	return func(options *ModifyClusterOptions) {
		options.Timeout.Set(d)
	}
}

type DeleteClusterTaskTypes struct {
	Delete   tasks.Type
	Metadata tasks.Type
	Purge    tasks.Type
}

func (t DeleteClusterTaskTypes) Validate() error {
	if t.Delete == "" {
		return xerrors.New("no delete task type set")
	}

	if t.Metadata == "" {
		return xerrors.New("no delete metadata task type set")
	}

	// TODO: this comment must be removed when all databases use purge tasks
	/*if t.Purge == "" {
		return xerrors.New("no purge task type set")
	}*/

	return nil
}

type DeleteClusterS3Buckets map[string]string

type DeleteClusterOptions struct {
	TaskArgs map[string]interface{}
}

type DeleteClusterOption func(options *DeleteClusterOptions)

func DeleteTaskArgs(a map[string]interface{}) DeleteClusterOption {
	return func(options *DeleteClusterOptions) {
		options.TaskArgs = a
	}
}

type StartClusterOptions struct {
	TaskArgs map[string]interface{}
}

type StartClusterOption func(options *StartClusterOptions)

func StartClusterTaskArgs(a map[string]interface{}) StartClusterOption {
	return func(options *StartClusterOptions) {
		options.TaskArgs = a
	}
}

type MoveClusterOptions struct {
	TaskArgs          map[string]interface{}
	SrcFolderTaskType tasks.Type
	DstFolderTaskType tasks.Type
	SrcFolderCoords   metadb.FolderCoords
	DstFolderExtID    string
}

type MoveClusterOption func(options *MoveClusterOptions)

type BackupClusterOptions struct {
	TaskArgs map[string]interface{}
}

type BackupClusterOption func(options *BackupClusterOptions)

func BackupClusterTaskArgs(a map[string]interface{}) BackupClusterOption {
	return func(options *BackupClusterOptions) {
		options.TaskArgs = a
	}
}

type UpgradeClusterOptions struct {
	TaskArgs map[string]interface{}
}

type UpgradeClusterOption func(options *UpgradeClusterOptions)

func UpgradeClusterTaskArgs(a map[string]interface{}) UpgradeClusterOption {
	return func(options *UpgradeClusterOptions) {
		options.TaskArgs = a
	}
}
