package gpmodels

import (
	"fmt"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	bmodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/backups"
)

const BackupPathDelimiter = "/"

var BackupMetaFileSuffix = "backup_stop_sentinel.json"

type BackupMeta struct {
	UserData   UserData  `json:"user_data"`
	StartTime  time.Time `json:"start_time"`
	FinishTime time.Time `json:"finish_time"`
	Size       int64     `json:"compressed_size"`
}

type UserData struct {
	BackupID string `json:"backup_id"`
}

func CalculateBackupPrefix(cid, version string) string {
	return fmt.Sprintf("wal-e/%s/%s/basebackups_005/", cid, version)
}

func BackupFromBackupMeta(backupMeta BackupMeta) bmodels.Backup {
	return bmodels.Backup{
		ID:        backupMeta.UserData.BackupID,
		StartedAt: optional.NewTime(backupMeta.StartTime),
		CreatedAt: backupMeta.FinishTime,
		Size:      backupMeta.Size,
	}
}
