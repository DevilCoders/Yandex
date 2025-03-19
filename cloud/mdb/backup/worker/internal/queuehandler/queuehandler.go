package queuehandler

import (
	"a.yandex-team.ru/cloud/mdb/backup/worker/internal/models"
)

//go:generate ../../../../scripts/mockgen.sh QueueHandler

type QueueHandler interface {
	HandleQueue(chan models.BackupJob) chan error
}
