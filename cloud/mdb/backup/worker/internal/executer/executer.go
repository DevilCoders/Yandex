package executer

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/backup/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/backup/worker/internal/models"
)

//go:generate ../../../../scripts/mockgen.sh Executer

type Resulter interface {
	private()
}

type BackupCreationStart struct {
	ShipmentID string
	Err        error
}

func (BackupCreationStart) private() {}

type BackupCreationComplete struct {
	Metadata metadb.BackupMetadata
	Err      error
}

func (BackupCreationComplete) private() {}

type BackupDeletionStart struct {
	ShipmentID string
	Err        error
}

func (BackupDeletionStart) private() {}

type BackupDeletionComplete struct {
	Err error
}

func (BackupDeletionComplete) private() {}

type BackupResize struct {
	ID          string
	DataSize    int64
	JournalSize int64
}

func (BackupResize) private() {}

var _ = []Resulter{
	BackupCreationStart{},
	BackupCreationComplete{},
	BackupDeletionStart{},
	BackupDeletionComplete{},
	BackupResize{},
}

type ExecFunc func(context.Context, models.BackupJob) (results []Resulter)

type Executer interface {
	StartCreation(context.Context, models.BackupJob) (results []Resulter)
	CompleteCreating(context.Context, models.BackupJob) (results []Resulter)
	StartDeletion(context.Context, models.BackupJob) (results []Resulter)
	CompleteDeleting(context.Context, models.BackupJob) (results []Resulter)
}
