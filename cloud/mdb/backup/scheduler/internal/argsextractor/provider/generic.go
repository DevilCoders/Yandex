package provider

import (
	"context"
	"math/rand"
	"time"

	"a.yandex-team.ru/cloud/mdb/backup/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/internal/generator"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type GenericBackupArgsExtractor struct{}

func NewGenericBackupArgsExtractor() *GenericBackupArgsExtractor {
	return &GenericBackupArgsExtractor{}
}

func (gbg *GenericBackupArgsExtractor) GetBackupArgs(ctx context.Context, blank metadb.BackupBlank, backupIDGen generator.IDGenerator) (metadb.CreateBackupArgs, error) {
	backupArgs := metadb.CreateBackupArgs{
		ClusterID:    blank.ClusterID,
		ClusterType:  blank.ClusterType,
		SubClusterID: blank.SubClusterID,
		ShardID:      blank.ShardID,
		DelayedUntil: blank.ScheduledTS.Add(time.Duration(rand.Intn(blank.SleepSeconds+1)) * time.Second),
		ScheduledAt:  optional.NewTime(blank.ScheduledTS),
		Initiator:    metadb.BackupInitiatorSchedule,
		Method:       metadb.BackupMethodFull, // Only FULL backup is supported for now
	}
	id, err := backupIDGen.Generate()
	if err != nil {
		return metadb.CreateBackupArgs{}, xerrors.Errorf("backup id not generated: %w", err)
	}

	backupArgs.BackupID = id
	return backupArgs, nil
}
