package clickhouse

import (
	"context"
	"strings"

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
	cid   string
	shard string
	meta  backupmanagerProv.ClickhouseMetadata
}

type Importer struct {
	bm       *backupmanagerProv.ClickhouseS3BackupManager
	mdb      metadb.MetaDB
	s3Client s3.Client
	idGen    generator.IDGenerator
	lg       log.Logger
}

func NewImporter(
	bm *backupmanagerProv.ClickhouseS3BackupManager,
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

func (imp *Importer) clusterMaps(ctx context.Context, cid string) (string, map[string]string, error) {
	shards, err := imp.mdb.ListShards(ctx, cid)
	if err != nil {
		return "", nil, err
	}
	var subclusterID string
	shardNameID := make(map[string]string)

	for _, shard := range shards {
		if shard.ID.Valid {
			shardNameID[shard.Name.String] = shard.ID.String
		}
		subclusterID = shard.SubClusterID
	}

	return subclusterID, shardNameID, nil
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

	existedCompIDs, err := backupsFromManaged(managedBackups)
	if err != nil {
		return stats, err
	}

	subcid, shardNameID, err := imp.clusterMaps(ctx, cid)
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

		backupID, err := imp.idGen.Generate()
		if err != nil {
			return stats, xerrors.Errorf("can not generate id: %w", err)
		}
		importArgs, err := importArgsFromBackup(backupID, meta, shardNameID, cid, subcid)
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

func importArgsFromBackup(backupID string, backup BackupMeta, shardName2ID map[string]string, cid string, subcid string) (metadb.ImportBackupArgs, error) {
	shardID := shardName2ID[backup.shard]

	importArgs := metadb.ImportBackupArgs{
		BackupID:     backupID,
		ClusterID:    cid,
		SubClusterID: subcid,
		ShardID:      optional.NewString(shardID),
		Initiator:    metadb.BackupInitiatorSchedule,
		Method:       metadb.BackupMethodFull,
		Status:       metadb.BackupStatusDone,
		DelayedUntil: backup.meta.StartTime,
		CreatedAt:    backup.meta.StartTime,
		ScheduledAt:  optional.NewTime(backup.meta.StartTime),
		StartedAt:    backup.meta.StartTime,
		UpdatedAt:    backup.meta.FinishTime,
		FinishedAt:   backup.meta.FinishTime,
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

	clusterPath := backupmanagerProv.BackupPathForCluster(cid)

	imp.lg.Debugf("Cluster path for %q cluster: %s", cid, clusterPath)

	_, shardPaths, err := imp.s3Client.ListObjects(ctx, bucket,
		s3.ListObjectsOpts{Prefix: ptr.String(clusterPath), Delimiter: ptr.String("/")})
	if err != nil {
		return nil, nil, err
	}

	imp.lg.Debugf("Found %d shards", len(shardPaths))

	shardNames := make([]string, len(shardPaths))
	for i := range shardPaths {
		prefix := shardPaths[i].Prefix
		imp.lg.Debugf("Prefix is  %q", prefix)
		if prefix[len(prefix)-1] == '/' {
			prefix = prefix[:len(prefix)-1]
		}
		shardNames[i] = prefix[strings.LastIndex(prefix, "/")+1:]
	}

	imp.lg.Debugf("Shard names for %q cluster: %+v", cid, shardNames)

	return imp.shardBackups(ctx, cid, shardNames, bucket)
}

func (imp *Importer) shardBackups(ctx context.Context, cid string, shards []string, bucket string) (map[BackupCompID]BackupMeta, []BackupCompID, error) {
	storageBackups := make(map[BackupCompID]BackupMeta)
	var order []BackupCompID
	for _, shard := range shards {

		shardBackups, err := imp.bm.ListBackups(ctx, bucket, backupmanagerProv.BackupPathForShard(cid, shard))
		if err != nil {
			return nil, nil, err
		}
		imp.lg.Debugf("Listed for shard (%q, %q) backups: %+v\n", cid, shard, shardBackups)

		for _, backup := range shardBackups {
			chMeta, ok := backup.(backupmanagerProv.ClickhouseMetadata)
			if !ok {
				return nil, nil, xerrors.Errorf("can not cast to mongo meta")
			}

			compID := BackupCompID{name: chMeta.BackupName, root: chMeta.RootPath}
			meta := BackupMeta{
				cid:   cid,
				shard: shard,
				meta:  chMeta,
			}
			storageBackups[compID] = meta
			order = append(order, compID)

		}
	}

	return storageBackups, order, nil
}

func backupsFromManaged(managedBackups []metadb.Backup) (map[BackupCompID]metadb.Backup, error) {
	compIDs := make(map[BackupCompID]metadb.Backup)
	for _, backup := range managedBackups {
		meta, err := backupmanagerProv.UnmarshalClickhouseMetadata(backup.Metadata)
		if err != nil {
			return nil, err
		}
		compIDs[BackupCompID{name: meta.BackupName, root: meta.RootPath}] = backup
	}
	return compIDs, nil
}
