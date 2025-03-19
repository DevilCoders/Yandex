package jobproducer

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/backup/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/backup/worker/internal/models"
)

//go:generate ../../../../scripts/mockgen.sh JobProducer

type JobProducer interface {
	ProduceBackupJob(ctx context.Context, ch chan models.BackupJob) (err error)
}

type ProduceBackupJobFunc func(ctx context.Context) (metadb.Backup, error)
