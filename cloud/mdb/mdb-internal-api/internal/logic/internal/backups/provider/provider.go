package provider

import (
	"context"
	"encoding/json"
	"sort"

	"a.yandex-team.ru/cloud/mdb/internal/generator"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/s3"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/backups"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	bmodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/backups"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type Backups struct {
	metaDB            metadb.Backend
	s3client          s3.Client
	cfg               logic.Config
	backupIDGenerator generator.IDGenerator
}

var _ backups.Backups = &Backups{}

func NewBackups(metaDB metadb.Backend, s3client s3.Client, backupIDGenerator generator.IDGenerator, cfg logic.Config) *Backups {
	return &Backups{metaDB: metaDB, s3client: s3client, cfg: cfg, backupIDGenerator: backupIDGenerator}
}

func (b *Backups) AddBackupSchedule(ctx context.Context, cid string, schedule bmodels.BackupSchedule, revision int64) error {
	if err := b.metaDB.AddBackupSchedule(ctx, cid, schedule, revision); err != nil {
		return xerrors.Errorf("failed to add cluster backup schedule to metadb: %w", err)
	}
	return nil
}

func (b *Backups) BackupsByClusterID(ctx context.Context, cid string, impl backups.ListS3Backups,
	pageToken bmodels.BackupsPageToken, pageSize optional.Int64) ([]bmodels.Backup, bmodels.BackupsPageToken, error) {
	gatheredBackups, err := impl(ctx, b.s3client, cid)
	if err != nil {
		return nil, bmodels.BackupsPageToken{}, err
	}
	return paginateBackups(gatheredBackups, pageSize, pageToken)
}

func (b *Backups) BackupsByFolderID(ctx context.Context, folderID int64, typ clusters.Type, impl backups.ListS3Backups,
	pageToken bmodels.BackupsPageToken, pageSize int64) ([]bmodels.Backup, bmodels.BackupsPageToken, error) {
	gatheredClusters, err := b.metaDB.Clusters(ctx, models.ListClusterArgs{
		FolderID:    folderID,
		ClusterType: typ,
		Visibility:  models.VisibilityVisibleOrDeleted,
		Limit:       optional.Int64{},
	})
	if err != nil {
		return nil, bmodels.BackupsPageToken{}, err
	}
	var gatheredBackups []bmodels.Backup
	for _, c := range gatheredClusters {
		clusterBackups, err := impl(ctx, b.s3client, c.ClusterID)
		if err != nil {
			return nil, bmodels.BackupsPageToken{}, err
		}
		if len(clusterBackups) > 0 {
			gatheredBackups = append(gatheredBackups, clusterBackups...)
		}
	}
	return paginateBackups(gatheredBackups, optional.NewInt64(pageSize), pageToken)
}

func (b *Backups) BackupByClusterIDBackupID(ctx context.Context, cid, backupID string, impl backups.ListS3Backups) (bmodels.Backup, error) {
	gatheredBackups, _, err := b.BackupsByClusterID(ctx, cid, impl, bmodels.BackupsPageToken{}, optional.Int64{})
	if err != nil {
		return bmodels.Backup{}, err
	}
	for _, backup := range gatheredBackups {
		if backup.ID == backupID {
			return backup, nil
		}
	}
	return bmodels.Backup{}, semerr.NotFoundf("backup %q does not exist", bmodels.EncodeGlobalBackupID(cid, backupID))
}

func (b *Backups) ScheduleBackupForNow(ctx context.Context, cid string, subcid string, shardID string, backupMethod bmodels.BackupMethod, metadata interface{}) (string, error) {
	backupID, err := b.GenerateNewBackupID()
	if err != nil {
		return "", xerrors.Errorf("backup id not generated: %w", err)
	}
	metadataBytes, err := json.Marshal(metadata)
	if err != nil {
		return "", err
	}
	err = b.metaDB.ScheduleBackupForNow(ctx, backupID, cid, subcid, shardID, backupMethod, metadataBytes)
	if err != nil {
		return "", xerrors.Errorf("failed to schedule backup: %w", err)
	}
	return backupID, nil
}

func (b *Backups) ManagedBackupByBackupID(ctx context.Context, backupID string, converter bmodels.BackupConverter) (bmodels.Backup, error) {
	managedBackup, err := b.metaDB.BackupByID(ctx, backupID)
	if err != nil {
		return bmodels.Backup{}, err
	}
	return converter.ManagedToRegular(managedBackup)
}

func (b *Backups) ManagedBackupsByClusterID(ctx context.Context, clusterID string, statuses []bmodels.BackupStatus, converter bmodels.BackupConverter) ([]bmodels.Backup, error) {
	bs, err := b.metaDB.BackupsByClusterID(ctx, clusterID)
	if err != nil {
		return nil, err
	}
	var managedBackups []bmodels.ManagedBackup
	for _, b := range bs {
		for _, s := range statuses {
			if b.Status == s {
				managedBackups = append(managedBackups, b)
			}
		}
	}
	return backupsFromManagedBackups(managedBackups, converter)
}

func (b *Backups) MarkManagedBackupObsolete(ctx context.Context, backupID string) error {
	return b.metaDB.MarkBackupObsolete(ctx, backupID)
}

func (b *Backups) GenerateNewBackupID() (string, error) {
	return b.backupIDGenerator.Generate()
}

func backupsFromManagedBackups(mbs []bmodels.ManagedBackup, converter bmodels.BackupConverter) ([]bmodels.Backup, error) {
	var res []bmodels.Backup
	for _, mb := range mbs {
		b, err := converter.ManagedToRegular(mb)
		if err != nil {
			return res, xerrors.Errorf("Can not cast managed backup to backup %s", err)
		}
		res = append(res, b)
	}
	return res, nil
}

func paginateBackups(gatheredBackups []bmodels.Backup, pageSize optional.Int64, pageToken bmodels.BackupsPageToken) ([]bmodels.Backup, bmodels.BackupsPageToken, error) {
	sort.Slice(gatheredBackups, func(i, j int) bool {
		return gatheredBackups[i].CreatedAt.After(gatheredBackups[j].CreatedAt)
	})

	if pageSize.Valid {
		page, nextPageToken := paginate(gatheredBackups, pageToken, pageSize.Int64)
		return page, nextPageToken, nil
	} else {
		return gatheredBackups, bmodels.BackupsPageToken{}, nil
	}
}
