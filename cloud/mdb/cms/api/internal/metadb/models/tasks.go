package models

import (
	"time"

	as "a.yandex-team.ru/cloud/mdb/internal/compute/accessservice"
	"a.yandex-team.ru/cloud/mdb/internal/idempotence"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type Operation struct {
	OperationID string
	TargetID    string
	ClusterID   string
	ClusterType string
	Environment string
	Type        string
	CreatedBy   string
	CreatedAt   time.Time
	StartedAt   time.Time
	ModifiedAt  time.Time
	Status      string
}

// CreateTaskArgs contains all necessary arguments for creation of worker tasks
type CreateTaskArgs struct {
	TaskID              string
	ClusterID           string
	FolderID            int64
	OperationType       string
	TaskType            string
	TaskArgs            map[string]interface{}
	Metadata            interface{}
	Auth                as.Subject
	Hidden              bool
	Timeout             optional.Duration
	Idempotence         *idempotence.Incoming
	SkipIdempotence     bool
	DelayBy             optional.Duration
	RequiredOperationID optional.String
	Revision            int64
}

func (cta CreateTaskArgs) Validate() error {
	if cta.ClusterID == "" {
		return xerrors.New("empty cluster id")
	}

	if cta.FolderID == 0 {
		return xerrors.New("zero folder id")
	}

	if cta.Auth.IsEmpty() {
		return xerrors.New("empty auth")
	}

	if cta.TaskType == "" {
		return xerrors.New("empty task type")
	}

	if cta.OperationType == "" {
		return xerrors.New("empty operation type")
	}

	if cta.Revision == 0 {
		return xerrors.New("zero revision")
	}

	return nil
}
