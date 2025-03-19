package postgresql

import (
	"context"
	"encoding/json"
	"sort"

	"github.com/jackc/pgerrcode"

	"a.yandex-team.ru/cloud/mdb/backup/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/backup/internal/tools/walg/postgresql"
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

type BackupCompID string

type BackupMeta struct {
	cid  string
	meta postgresql.PostgreSQLMetadata
}

type Importer struct {
	bm       *backupmanagerProv.PostgreSQLS3BackupManager
	mdb      metadb.MetaDB
	s3Client s3.Client
	idGen    generator.IDGenerator
	lg       log.Logger
}

func NewImporter(
	bm *backupmanagerProv.PostgreSQLS3BackupManager,
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

func (imp *Importer) Import(ctx context.Context, cid string, skipSchedDateDups, completeFailed, dryrun bool) (importer.ImportStats, error) {
	stats := importer.ImportStats{}

	storageBackups, storageOrder, err := imp.backupsFromStorage(ctx, cid)
	if err != nil {
		return stats, err
	}
	stats.ExistsInStorage = len(storageBackups)
	imp.lg.Debugf("Found %d backups in storage", len(storageBackups))

	allMetaDBBackups, err := imp.mdb.ListBackups(ctx, cid, optional.String{}, optional.String{}, metadb.AllStatuses, []metadb.BackupInitiator{metadb.BackupInitiatorSchedule, metadb.BackupInitiatorUser})
	if err != nil {
		return stats, err
	}
	stats.ExistsInMetadb = len(allMetaDBBackups)
	imp.lg.Debugf("Found %d backups in metadb", stats.ExistsInMetadb)

	completedBackupNames, existedBackupIDs, err := backupsFromManaged(allMetaDBBackups)
	if err != nil {
		return stats, err
	}

	for i := range storageOrder {
		err = imp.importSingleBackup(ctx, &stats, storageOrder[i], cid, storageBackups,
			completedBackupNames, existedBackupIDs, dryrun, skipSchedDateDups, completeFailed)
		if err != nil {
			return stats, err
		}
	}
	return stats, nil
}

func (imp *Importer) importSingleBackup(
	ctx context.Context, stats *importer.ImportStats,
	backup BackupCompID, cid string,
	storageBackups map[BackupCompID]BackupMeta,
	existedCompIDs map[BackupCompID]metadb.Backup, existedBackupIDs map[string]metadb.Backup,
	dryrun, skipSchedDateDups, completeFailed bool,
) error {
	meta := storageBackups[backup]

	if exBackup, ok := imp.findExistingBackup(backup, meta, existedCompIDs, existedBackupIDs); ok {
		return imp.handleExistingBackup(ctx, stats, backup, exBackup, meta, completeFailed)
	}

	backupID, err := imp.idGen.Generate()
	if err != nil {
		return xerrors.Errorf("can not generate id: %w", err)
	}
	importArgs, err := imp.importArgsFromBackup(ctx, backupID, meta, cid)
	if err != nil {
		return xerrors.Errorf("can not build import args for backup %+v: %w", backup, err)
	}

	if meta.meta.IsIncremental {
		parentBackupID, err := findIncrementBase(meta, existedCompIDs)
		if err != nil {
			imp.lg.Debugf("Skipping incremental backup %s, could not find increment base: %+v", backup, err)
			stats.SkippedNoIncrementBase++
			return nil
		}
		importArgs.DependsOnBackupIDs = []string{parentBackupID}
	}
	imp.lg.Debugf("Built backup %+v for import with args:%+v\n", backup, importArgs)

	if dryrun {
		dryBackup := imp.dryImportBackup(importArgs)
		existedBackupIDs[backupID] = dryBackup
		existedCompIDs[backup] = dryBackup
		imp.lg.Debugf("Skipping metadb import of backup %+v due to dry-run flag", backup)
		stats.SkippedDueToDryRun++
		return nil
	}
	txCtx, err := imp.mdb.Begin(ctx, sqlutil.Primary)
	if err != nil {
		return err
	}

	importedBackup, err := imp.mdb.ImportBackup(txCtx, importArgs)

	if err != nil {
		code, _ := pgerrors.Code(err)
		if skipSchedDateDups && code == pgerrcode.UniqueViolation {
			stats.SkippedDueToUniqSchedDate++
			imp.lg.Debugf("Skipping backup due to scheduled date unique violation: %+v", err)
			return nil
		}
		return imp.mdb.Rollback(txCtx)
	}
	err = imp.mdb.Commit(txCtx)
	if err != nil {
		return err
	}

	existedBackupIDs[importedBackup.BackupID] = importedBackup
	existedCompIDs[backup] = importedBackup
	stats.ImportedIntoMetadb++
	imp.lg.Debugf("Successfully imported backup %s into metadb", importedBackup.BackupID)
	return nil
}

func (imp *Importer) findExistingBackup(backup BackupCompID, meta BackupMeta, existedCompIDs map[BackupCompID]metadb.Backup, existedBackupIDs map[string]metadb.Backup) (metadb.Backup, bool) {
	// Possible scenarios:
	// 1. Backup exists in metadb with the same backup ID => true
	// 2. Backup exists in metadb with the different backup ID but with the same name => true
	// 3. Otherwise => false

	if exBackup, ok := existedBackupIDs[meta.meta.BackupID]; ok {
		return exBackup, true
	}

	if exBackup, ok := existedCompIDs[backup]; ok {
		return exBackup, true
	}

	return metadb.Backup{}, false
}

func (imp *Importer) handleExistingBackup(ctx context.Context,
	stats *importer.ImportStats, backup BackupCompID, exBackup metadb.Backup, meta BackupMeta, completeFailed bool) error {
	if completeFailed && exBackup.Status == metadb.BackupStatusCreateError {
		// if we try to import this backup, it means that it has been created successfully
		// if it has the failed status in metadb, we should mark it as completed
		txCtx, err := imp.mdb.Begin(ctx, sqlutil.Primary)
		if err != nil {
			return err
		}

		finishTime := optional.NewTime(meta.meta.FinishTime)
		err = imp.mdb.CompleteBackupCreation(txCtx, exBackup.BackupID, finishTime, meta.meta)
		if err == nil {
			stats.CompletedCreation++
			imp.lg.Debugf("Successfully completed backup %s (backup id: %s) creation", backup, exBackup.BackupID)

			return imp.mdb.Commit(txCtx)
		}

		imp.lg.Debugf("Failed to complete backup %s (backup id: %s) creation: %v", backup, exBackup.BackupID, err)
		_ = imp.mdb.Rollback(txCtx)
		return err
	}

	imp.lg.Debugf("Skipping import of backup %+v because it already exists in metadb: %+v", backup, exBackup)
	stats.SkippedDueToExistence++
	return nil
}

func findIncrementBase(meta BackupMeta, existedCompIDs map[BackupCompID]metadb.Backup) (string, error) {
	incrementBase := BackupCompID(meta.meta.IncrementDetails.IncrementFrom)
	parentBackup, ok := existedCompIDs[incrementBase]
	if !ok {
		return "", xerrors.Errorf("%s backup (increment base) was not found in existedCompIDs", incrementBase)
	}
	return parentBackup.BackupID, nil
}

func (imp *Importer) importArgsFromBackup(ctx context.Context, backupID string, backup BackupMeta, cid string) (metadb.ImportBackupArgs, error) {
	meta, err := postgresql.UnmarshalExtendedMetadataDto(backup.meta.RawMeta)
	if err != nil {
		return metadb.ImportBackupArgs{}, err
	}

	meta.UserData.BackupID = backupID

	if backup.meta.RawMeta, err = json.Marshal(meta); err != nil {
		return metadb.ImportBackupArgs{}, err
	}

	initiator := metadb.BackupInitiatorSchedule
	scheduledAt := optional.NewTime(meta.StartTime)
	if meta.IsPermanent {
		initiator = metadb.BackupInitiatorUser
		scheduledAt = optional.Time{}
	}

	scs, err := imp.mdb.ListSubClusters(ctx, cid)
	if err != nil {
		return metadb.ImportBackupArgs{}, err
	}

	method := metadb.BackupMethodFull

	if backup.meta.IsIncremental {
		method = metadb.BackupMethodIncremental
	}

	if meta.UserData.BackupID == "" {
		backup.meta.BackupID = backupID
	} else {
		backup.meta.BackupID = meta.UserData.BackupID
	}

	importArgs := metadb.ImportBackupArgs{
		BackupID:     backupID,
		ClusterID:    cid,
		SubClusterID: scs[0].SubClusterID,
		Initiator:    initiator,
		Method:       method,
		Status:       metadb.BackupStatusDone,
		DelayedUntil: meta.StartTime,
		CreatedAt:    meta.StartTime,
		StartedAt:    meta.StartTime,
		UpdatedAt:    meta.FinishTime,
		FinishedAt:   meta.FinishTime,
		ScheduledAt:  scheduledAt,
		Metadata:     backup.meta,
	}
	return importArgs, nil
}

func (imp *Importer) backupsFromStorage(ctx context.Context, cid string) (map[BackupCompID]BackupMeta, []BackupCompID, error) {
	imp.lg.Debugf("Importing s3 backups for %q cluster", cid)
	bucket, err := imp.mdb.ClusterBucket(ctx, cid)
	if err != nil {
		return nil, nil, metadb.WrapTempError(err, "can not get cluster bucket")
	}
	imp.lg.Debugf("S3 bucket for %q cluster: %s", cid, bucket)

	pgVersion, err := backupmanagerProv.PostgresMajorVersion(ctx, imp.mdb, cid)
	if err != nil {
		return nil, nil, metadb.WrapTempError(err, "can not get cluster Postgres major version")
	}
	imp.lg.Debugf("Postgres major version for %q cluster: %d", cid, pgVersion)

	storageBackups := make(map[BackupCompID]BackupMeta)

	backups, err := imp.bm.ListBackupsAllVersions(ctx, bucket, backupmanagerProv.GetPostgreSQLWalgBackupsPath(cid))
	if err != nil {
		return nil, nil, err
	}
	imp.lg.Debugf("Listed from %v backups: %+v\n", cid, backups)

	for _, backup := range backups {
		postgresqlMeta, ok := backup.(postgresql.PostgreSQLMetadata)
		if !ok {
			return nil, nil, xerrors.Errorf("can not cast to postgresql meta")
		}

		compID := BackupCompID(postgresqlMeta.BackupName)
		meta := BackupMeta{
			cid:  cid,
			meta: postgresqlMeta,
		}
		storageBackups[compID] = meta
	}

	return storageBackups, sortStorageBackups(storageBackups), nil
}

func (imp *Importer) dryImportBackup(importArgs metadb.ImportBackupArgs) metadb.Backup {
	metaBytes, err := importArgs.Metadata.Marshal()
	if err != nil {
		metaBytes = nil
		imp.lg.Warnf("dryImportBackup: failed to marshal metadata for backup %s, leaving empty", importArgs.BackupID)
	}
	return metadb.Backup{
		BackupID:      importArgs.BackupID,
		ClusterID:     importArgs.ClusterID,
		SubClusterID:  importArgs.SubClusterID,
		ShardID:       importArgs.ShardID,
		Method:        importArgs.Method,
		Initiator:     importArgs.Initiator,
		Status:        importArgs.Status,
		CreatedAt:     importArgs.CreatedAt,
		DelayedUntil:  importArgs.DelayedUntil,
		StartedAt:     optional.Time{Time: importArgs.StartedAt, Valid: true},
		UpdatedAt:     optional.Time{Time: importArgs.UpdatedAt, Valid: true},
		FinishedAt:    optional.Time{Time: importArgs.FinishedAt, Valid: true},
		ScheduledDate: importArgs.ScheduledAt,
		Metadata:      metaBytes,
	}
}

// sortStorageBackups puts base backups before incremental, and sorts backups by the creation time
func sortStorageBackups(backups map[BackupCompID]BackupMeta) []BackupCompID {
	orderedCompIDs := make([]BackupCompID, 0, len(backups))
	for compID := range backups {
		orderedCompIDs = append(orderedCompIDs, compID)
	}
	sort.Slice(orderedCompIDs, func(i, j int) bool {
		iMeta, jMeta := backups[orderedCompIDs[i]].meta, backups[orderedCompIDs[j]].meta

		switch {
		// if both backups are incremental, order by start time
		case iMeta.IsIncremental && jMeta.IsIncremental:
			return iMeta.StartTime.Before(jMeta.StartTime)

		// if i'th backup is incremental, it should go after base backup (j'th)
		case iMeta.IsIncremental:
			return false

		// if j'th backup is incremental, it should go after base backup (i'th)
		case jMeta.IsIncremental:
			return true

		// if both backups are base backups, order by start time
		default:
			return iMeta.StartTime.Before(jMeta.StartTime)
		}
	})
	return orderedCompIDs
}

func backupsFromManaged(managedBackups []metadb.Backup) (map[BackupCompID]metadb.Backup, map[string]metadb.Backup, error) {
	compIDs := make(map[BackupCompID]metadb.Backup)
	backupIDs := make(map[string]metadb.Backup)
	for _, backup := range managedBackups {
		compID, ok, err := unwrapCreatedBackupInfo(backup)
		if err != nil {
			return nil, nil, err
		}
		if ok {
			compIDs[compID] = backup
		}
		backupIDs[backup.BackupID] = backup
	}
	return compIDs, backupIDs, nil
}

func unwrapCreatedBackupInfo(backup metadb.Backup) (BackupCompID, bool, error) {
	for _, status := range metadb.StatusesCreated {
		// if backup is in the "created" status, extract the backup name from metadata
		if backup.Status == status {
			meta, err := postgresql.UnmarshalPostgreSQLMetadata(backup.Metadata)
			if err != nil {
				return "", false, err
			}

			return BackupCompID(meta.BackupName), true, nil
		}
	}
	return "", false, nil
}
