package queueproducer

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/backup/worker/internal/models"
)

//go:generate ../../../../scripts/mockgen.sh QueueProducer

type QueueProducer interface {
	ProduceQueue(produceCtx, jobCtx context.Context) (chan models.BackupJob, chan error)
}
