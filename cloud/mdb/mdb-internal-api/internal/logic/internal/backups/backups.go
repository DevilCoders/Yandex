package backups

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/s3"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/backups"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
)

//go:generate ../../../../../scripts/mockgen.sh Backups

type Backups interface {
	AddBackupSchedule(ctx context.Context, cid string, schedule backups.BackupSchedule, revision int64) error

	BackupsByClusterID(ctx context.Context, cid string, impl ListS3Backups, pageToken backups.BackupsPageToken, pageSize optional.Int64) ([]backups.Backup, backups.BackupsPageToken, error)
	BackupsByFolderID(ctx context.Context, folderID int64, typ clusters.Type, impl ListS3Backups, pageToken backups.BackupsPageToken, pageSize int64) ([]backups.Backup, backups.BackupsPageToken, error)
	BackupByClusterIDBackupID(ctx context.Context, cid, backupID string, impl ListS3Backups) (backups.Backup, error)
	ScheduleBackupForNow(ctx context.Context, cid string, subcid string, shardID string, backupMethod backups.BackupMethod, metadata interface{}) (string, error)
	ManagedBackupByBackupID(ctx context.Context, backupID string, converter backups.BackupConverter) (backups.Backup, error)
	ManagedBackupsByClusterID(ctx context.Context, clusterID string, statuses []backups.BackupStatus, converter backups.BackupConverter) ([]backups.Backup, error)
	MarkManagedBackupObsolete(ctx context.Context, backupID string) error
	GenerateNewBackupID() (string, error)
}

type ListS3Backups func(ctx context.Context, s3client s3.Client, cid string) ([]backups.Backup, error)
