package provider

import (
	"a.yandex-team.ru/cloud/mdb/backup/internal/metadb"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type SplittableBackup interface {
	ID() string
	Initiator() metadb.BackupInitiator
}

// NextAutoBackupIndexes finds requested and next automated backups for given ID and manual backups between them
func NextAutoBackupIndexes[T SplittableBackup](backups []T, backupID string) (requested int, intermManual []int, nextAuto int, err error) {
	requested = -1
	nextAuto = -1

	for i := range backups {
		if requested == -1 {
			if backups[i].ID() == backupID {
				requested = i
			}
			continue
		}

		if backups[i].Initiator() == metadb.BackupInitiatorSchedule {
			nextAuto = i
			break
		}
		intermManual = append(intermManual, i)
	}
	if requested == -1 {
		return -1, nil, -1, xerrors.Errorf("backup %s was not found", backupID)
	}

	if nextAuto == -1 {
		intermManual = nil
	}
	return requested, intermManual, nextAuto, nil
}

// PrevAutoBackup finds previos automated backup index
func PrevAutoBackup[V SplittableBackup](sortedBackups []V, backupID string) int {
	prevAutoIdx := -1
	for i := range sortedBackups {
		if sortedBackups[i].ID() == backupID {
			return prevAutoIdx
		}
		if sortedBackups[i].Initiator() == metadb.BackupInitiatorSchedule {
			prevAutoIdx = i
		}
	}
	return -1
}
