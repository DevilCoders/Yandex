package jobhandler

import (
	"a.yandex-team.ru/cloud/mdb/backup/worker/internal/models"
)

//go:generate ../../../../scripts/mockgen.sh JobHandler

type JobHandler interface {
	HandleBackupJob(job models.BackupJob) error
}
