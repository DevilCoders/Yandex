package mongodb

import (
	"context"

	"github.com/jackc/pgerrcode"

	"a.yandex-team.ru/cloud/mdb/backup/internal/metadb"
	backupmanagerProv "a.yandex-team.ru/cloud/mdb/backup/worker/internal/backupmanager/provider"
	"a.yandex-team.ru/cloud/mdb/backup/worker/internal/importer"
	"a.yandex-team.ru/cloud/mdb/internal/generator"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/s3"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/pgutil/pgerrors"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/ptr"
)

type BackupCompID struct {
	name string
	root string
}

type BackupMeta struct {
	cid         string
	replsetID   string
	replsetName string
	meta        backupmanagerProv.MongoDBMetadata
	sentinel    backupmanagerProv.MongoDBWalgSentinel
}

type Importer struct {
	bm       *backupmanagerProv.MongodbS3BackupManager
	mdb      metadb.MetaDB
	s3Client s3.Client
	idGen    generator.IDGenerator
	lg       log.Logger
}

func NewImporter(
	bm *backupmanagerProv.MongodbS3BackupManager,
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

func (imp *Importer) clusterMaps(ctx context.Context, cid string) (map[string]string, map[string]string, error) {
	shards, err := imp.mdb.ListShards(ctx, cid)
	if err != nil {
		return nil, nil, err
	}
	subclusterNameID := make(map[string]string)
	shardNameID := make(map[string]string)

	for _, shard := range shards {
		if shard.ID.Valid {
			shardNameID[shard.Name.String] = shard.ID.String
		}
		subclusterNameID[shard.SubClusterName] = shard.SubClusterID
	}

	return subclusterNameID, shardNameID, nil
}

func (imp *Importer) Import(ctx context.Context, cid string, skipSchedDateDups, dryrun bool) (importer.ImportStats, error) {
	stats := importer.ImportStats{}

	storageBackups, storageOrder, err := imp.backupsFromStorage(ctx, cid)
	if err != nil {
		return stats, err
	}
	stats.ExistsInStorage = len(storageBackups)
	imp.lg.Debugf("Found %d backups in storage", len(storageBackups))

	managedBackups, err := imp.mdb.ListBackups(ctx, cid, optional.String{}, optional.String{}, metadb.StatusesCreated, []metadb.BackupInitiator{metadb.BackupInitiatorSchedule, metadb.BackupInitiatorUser})
	if err != nil {
		return stats, err
	}
	stats.ExistsInMetadb = len(managedBackups)
	imp.lg.Debugf("Found %d backups in metadb", stats.ExistsInMetadb)

	existedCompIDs, existedBackupIDs, err := backupsFromManaged(managedBackups)
	if err != nil {
		return stats, err
	}

	subclusterNameID, shardNameID, err := imp.clusterMaps(ctx, cid)
	if err != nil {
		return stats, err
	}

	for i := range storageOrder {
		backup := storageOrder[i]
		meta := storageBackups[backup]
		if exBackup, ok := existedCompIDs[backup]; ok {
			imp.lg.Debugf("Skipping metadb import of backup %+v because it already exists in metadb: %+v", backup, exBackup)
			stats.SkippedDueToExistence++
			continue
		}

		// double check existence by id, fail if exists (we assume first check is enough)
		if exBackup, ok := existedBackupIDs[meta.sentinel.UserData.BackupID]; ok {
			return stats, xerrors.Errorf("backupID %q was found in metadb by ID but not by name+root %+v: check metadb backup %v",
				meta.sentinel.UserData.BackupID, backup, exBackup)
		}

		backupID, err := imp.idGen.Generate()
		if err != nil {
			return stats, xerrors.Errorf("can not generate id: %w", err)
		}
		importArgs, err := importArgsFromBackup(backupID, meta, subclusterNameID, shardNameID, imp.lg, cid)
		if err != nil {
			return stats, xerrors.Errorf("can not build import args for backup %+v: %w", backup, err)
		}

		imp.lg.Debugf("Built backup %+v for import with args:%+v\n", backup, importArgs)

		if dryrun {
			imp.lg.Debugf("Skipping metadb import of backup %+v due to dry-run flag", backup)
			stats.SkippedDueToDryRun++
			continue
		}

		txCtx, err := imp.mdb.Begin(ctx, sqlutil.Primary)
		if err != nil {
			return stats, err
		}
		_, err = imp.mdb.ImportBackup(txCtx, importArgs)
		if err != nil {
			code, _ := pgerrors.Code(err)
			if skipSchedDateDups && code == pgerrcode.UniqueViolation {
				stats.SkippedDueToUniqSchedDate++
				imp.lg.Debugf("Skipping backup due to scheduled date unique violation: %+v", err)
				continue
			}
			_ = imp.mdb.Rollback(txCtx)
			return stats, err
		}

		if err := imp.mdb.Commit(txCtx); err != nil {
			return stats, err
		}

		stats.ImportedIntoMetadb++

	}
	return stats, nil

}

func importArgsFromBackup(backupID string, backup BackupMeta, subclusterName2ID, shardName2ID map[string]string, lg log.Logger, cid string) (metadb.ImportBackupArgs, error) {
	sentinel, err := backupmanagerProv.UnmarshalMongoDBWalgSentinel(backup.meta.RawMeta)
	if err != nil {
		return metadb.ImportBackupArgs{}, err
	}

	initiator := metadb.BackupInitiatorSchedule
	scheduledAt := optional.NewTime(sentinel.StartLocalTime)
	if sentinel.Permanent {
		initiator = metadb.BackupInitiatorUser
		scheduledAt = optional.Time{}
	}

	var subclusterID string
	var shardID optional.String
	if backup.replsetName == backupmanagerProv.WalgMongocfgReplSetName {
		subclusterID = subclusterName2ID[metadb.MongocfgSubcluster]
	} else if backup.replsetName == backupmanagerProv.WalgMongoinfraReplSetName {
		subclusterID = subclusterName2ID[metadb.MongoinfraSubcluster]
	} else {
		subclusterID = subclusterName2ID[metadb.MongodSubcluster]
		shardIDstr, ok := shardName2ID[backup.replsetName]
		if !ok {
			lg.Errorf("shard_id for shard_name %q was not found, continue with empty shard_id", backup.replsetID)
		}
		if shardIDstr != backup.replsetID {
			return metadb.ImportBackupArgs{}, xerrors.Errorf("shard_id from replset != shard_id from shard_name: %s != %s", backup.replsetID, shardIDstr)
		}
		shardID = optional.NewString(shardIDstr)
	}

	importArgs := metadb.ImportBackupArgs{
		BackupID:     backupID,
		ClusterID:    cid,
		SubClusterID: subclusterID,
		ShardID:      shardID,
		Initiator:    initiator,
		Method:       metadb.BackupMethodFull,
		Status:       metadb.BackupStatusDone,
		DelayedUntil: sentinel.StartLocalTime,
		CreatedAt:    sentinel.StartLocalTime,
		StartedAt:    sentinel.StartLocalTime,
		UpdatedAt:    sentinel.FinishLocalTime,
		FinishedAt:   sentinel.FinishLocalTime,
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

	replsetRoots, err := replsetRootsFromBucket(ctx, cid, imp.s3Client, bucket)
	if err != nil {
		return nil, nil, err
	}
	imp.lg.Debugf("Replset roots %q cluster: %+v", cid, replsetRoots)

	return imp.backupsFromRoots(ctx, cid, replsetRoots, bucket)
}

func replsetRootsFromBucket(ctx context.Context, cid string, s3client s3.Client, bucket string) ([]string, error) {
	_, replsetPaths, err := s3client.ListObjects(ctx, bucket,
		s3.ListObjectsOpts{Prefix: ptr.String(backupmanagerProv.MongodbClusterBackupRoot(cid)), Delimiter: ptr.String("/")})
	if err != nil {
		return nil, err
	}

	replsetRoots := make([]string, len(replsetPaths))
	for i := range replsetPaths {
		replsetRoots[i] = replsetPaths[i].Prefix
	}
	return replsetRoots, nil
}

func (imp *Importer) backupsFromRoots(ctx context.Context, cid string, replsetRoots []string, bucket string) (map[BackupCompID]BackupMeta, []BackupCompID, error) {
	storageBackups := make(map[BackupCompID]BackupMeta)
	var order []BackupCompID
	for _, root := range replsetRoots {
		storageCid, replsetID, err := backupmanagerProv.MongodbIDsFromRoot(root)
		if err != nil {
			return nil, nil, err
		}

		if storageCid != cid {
			return nil, nil, xerrors.Errorf("given cluster_id != storage cluster_id: %s != %s", cid, storageCid)
		}

		replsetBackups, err := imp.bm.ListBackups(ctx, bucket, backupmanagerProv.MongodbBackupPathFromReplset(cid, replsetID))
		if err != nil {
			return nil, nil, err
		}
		imp.lg.Debugf("Listed from root %q (%q, %q) backups: %+v\n", root, storageCid, replsetID, replsetBackups)

		for _, rsbackup := range replsetBackups {
			mongoMeta, ok := rsbackup.(backupmanagerProv.MongoDBMetadata)
			if !ok {
				return nil, nil, xerrors.Errorf("can not cast to mongo meta")
			}

			sentinel, err := backupmanagerProv.UnmarshalMongoDBWalgSentinel(mongoMeta.RawMeta)
			if err != nil {
				return nil, nil, err
			}
			compID := BackupCompID{name: mongoMeta.BackupName, root: mongoMeta.RootPath}
			meta := BackupMeta{
				cid:         cid,
				replsetID:   replsetID,
				replsetName: mongoMeta.ShardNames[0],
				meta:        mongoMeta,
				sentinel:    sentinel,
			}
			storageBackups[compID] = meta
			order = append(order, compID)

		}
	}

	return storageBackups, order, nil
}

func backupsFromManaged(managedBackups []metadb.Backup) (map[BackupCompID]metadb.Backup, map[string]metadb.Backup, error) {
	compIDs := make(map[BackupCompID]metadb.Backup)
	backupIDs := make(map[string]metadb.Backup)
	for _, backup := range managedBackups {
		meta, err := backupmanagerProv.UnmarshalMongoDBMetadata(backup.Metadata)
		if err != nil {
			return nil, nil, err
		}
		compIDs[BackupCompID{name: meta.BackupName, root: meta.RootPath}] = backup
		backupIDs[backup.BackupID] = backup
	}
	return compIDs, backupIDs, nil
}
