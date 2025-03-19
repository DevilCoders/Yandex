package mysql

import (
	"context"

	"github.com/jackc/pgerrcode"

	"a.yandex-team.ru/cloud/mdb/backup/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/backup/internal/tools/walg/mysql"
	backupmanagerProv "a.yandex-team.ru/cloud/mdb/backup/worker/internal/backupmanager/provider"
	"a.yandex-team.ru/cloud/mdb/backup/worker/internal/importer"
	"a.yandex-team.ru/cloud/mdb/internal/generator"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/s3"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/pgutil/pgerrors"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type BackupMeta struct {
	cid  string
	meta mysql.MySQLMetadata
}

type Importer struct {
	bm       *backupmanagerProv.MySQLS3BackupManager
	mdb      metadb.MetaDB
	s3Client s3.Client
	idGen    generator.IDGenerator
	lg       log.Logger
}

func NewImporter(
	bm *backupmanagerProv.MySQLS3BackupManager,
	mdb metadb.MetaDB,
	s3Client s3.Client,
	idGen generator.IDGenerator,
	lg log.Logger,
) *Importer {
	return &Importer{bm, mdb, s3Client, idGen, lg}
}

func (imp *Importer) IsReady(ctx context.Context) error {
	return imp.mdb.IsReady(ctx)
}

func (imp *Importer) Import(ctx context.Context, cid string, skipSchedDateDups, dryrun bool) (importer.ImportStats, error) {
	stats := importer.ImportStats{}

	storageBackups, err := imp.backupsFromStorage(ctx, cid)
	if err != nil {
		return stats, err
	}
	stats.ExistsInStorage = len(storageBackups)
	imp.lg.Debug("found backups in storage:", log.Int("backups num", len(storageBackups)))

	managedBackups, err := imp.mdb.ListBackups(ctx, cid, optional.String{}, optional.String{}, metadb.StatusesCreated, []metadb.BackupInitiator{metadb.BackupInitiatorSchedule, metadb.BackupInitiatorUser})
	if err != nil {
		return stats, err
	}
	stats.ExistsInMetadb = len(managedBackups)
	imp.lg.Debug("found backups in metadb:", log.Int("metadb backups num", stats.ExistsInMetadb))

	existedBackups, err := backupsFromManaged(managedBackups)
	if err != nil {
		return stats, err
	}

	for _, meta := range storageBackups {
		backupName := meta.meta.BackupName
		if _, ok := existedBackups[backupName]; ok {
			imp.lg.Debug("skipping metadb import of backup because it already exists in metadb", log.String("backup_id", string(backupName)))
			stats.SkippedDueToExistence++
			continue
		}

		backupID, err := imp.idGen.Generate()
		if err != nil {
			return stats, xerrors.Errorf("can not generate id: %w", err)
		}
		importArgs, err := imp.importArgsFromBackup(ctx, backupID, meta, imp.lg, cid)
		if err != nil {
			return stats, xerrors.Errorf("can not build import args for backup %+v: %w", backupName, err)
		}

		if dryrun {
			imp.lg.Debug("skipping metadb import of backup due to dry-run flag")
			stats.SkippedDueToDryRun++
			continue
		}

		txCtx, err := imp.mdb.Begin(ctx, sqlutil.Primary)
		if err != nil {
			return stats, err
		}

		if _, err := imp.mdb.ImportBackup(txCtx, importArgs); err != nil {
			code, _ := pgerrors.Code(err)
			if skipSchedDateDups && code == pgerrcode.UniqueViolation {
				stats.SkippedDueToUniqSchedDate++
				imp.lg.Debug("skipping backup due to scheduled date unique violation:", log.Error(err))
				continue
			}
			if err := imp.mdb.Rollback(txCtx); err != nil {
				return stats, err
			}
			return stats, err
		}

		if err := imp.mdb.Commit(txCtx); err != nil {
			return stats, err
		}

		stats.ImportedIntoMetadb++
	}
	return stats, nil

}

func (imp *Importer) importArgsFromBackup(ctx context.Context, backupID string, backup BackupMeta, lg log.Logger, cid string) (metadb.ImportBackupArgs, error) {
	meta, err := mysql.UnmarshalStreamSentinelDto(backup.meta.RawMeta)
	if err != nil {
		return metadb.ImportBackupArgs{}, err
	}

	initiator := metadb.BackupInitiatorSchedule
	scheduledAt := optional.NewTime(meta.StartLocalTime)
	if meta.IsPermanent {
		initiator = metadb.BackupInitiatorUser
		scheduledAt = optional.Time{}
	}

	scs, err := imp.mdb.ListSubClusters(ctx, cid)
	if err != nil {
		return metadb.ImportBackupArgs{}, err
	}

	importArgs := metadb.ImportBackupArgs{
		BackupID:     backupID,
		ClusterID:    cid,
		SubClusterID: scs[0].SubClusterID,
		Initiator:    initiator,
		Method:       metadb.BackupMethodFull,
		Status:       metadb.BackupStatusDone,
		DelayedUntil: meta.StartLocalTime,
		CreatedAt:    meta.StartLocalTime,
		StartedAt:    meta.StartLocalTime,
		UpdatedAt:    meta.StopLocalTime,
		FinishedAt:   meta.StopLocalTime,
		ScheduledAt:  scheduledAt,
		Metadata:     backup.meta,
	}
	return importArgs, nil
}

func (imp *Importer) backupsFromStorage(ctx context.Context, cid string) ([]BackupMeta, error) {
	bucket, err := imp.mdb.ClusterBucket(ctx, cid)
	if err != nil {
		return nil, metadb.WrapTempError(err, "can not get cluster bucket")
	}

	myVersion, err := backupmanagerProv.ParseMysqlMajorVersion(ctx, imp.mdb, cid)
	if err != nil {
		return nil, metadb.WrapTempError(err, "can not get cluster mysql major version")
	}

	var storageBackups []BackupMeta

	backups, err := imp.bm.ListBackups(ctx, bucket,
		backupmanagerProv.MySQLBackupPathFromCidAndVersion(cid, myVersion))

	if err != nil {
		return nil, err
	}

	for _, backup := range backups {
		mysqlMeta, ok := backup.(mysql.MySQLMetadata)
		if !ok {
			return nil, xerrors.Errorf("can not cast to mysql meta")
		}

		meta := BackupMeta{
			cid:  cid,
			meta: mysqlMeta,
		}
		storageBackups = append(storageBackups, meta)
	}

	return storageBackups, nil
}

func backupsFromManaged(managedBackups []metadb.Backup) (map[string]struct{}, error) {
	backups := make(map[string]struct{})
	for _, backup := range managedBackups {
		meta, err := mysql.UnmarshalMySQLMetadata(backup.Metadata)
		backups[backup.BackupID] = struct{}{}
		if err != nil {
			return nil, err
		}
		backups[meta.BackupName] = struct{}{}
	}

	return backups, nil
}
