package provider

import (
	"context"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/s3"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/sessions"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/sqlserver/ssmodels"
	bmodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/backups"
	cmodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/pagination"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/tasks"
)

func (ss *SQLServer) BackupCluster(ctx context.Context, cid string) (operations.Operation, error) {
	return ss.operator.CreateOnCluster(ctx, cid, cmodels.TypeSQLServer,
		func(ctx context.Context, session sessions.Session, _ clusters.Reader, _ clusters.Modifier, cluster clusters.Cluster) (operations.Operation, error) {
			return ss.tasks.CreateTask(
				ctx,
				session,
				tasks.CreateTaskArgs{
					ClusterID:     cid,
					FolderID:      session.FolderCoords.FolderID,
					Auth:          session.Subject,
					TaskType:      ssmodels.TaskTypeClusterBackup,
					OperationType: ssmodels.OperationTypeClusterBackup,
					Revision:      cluster.Revision,
					Timeout:       optional.NewDuration(24 * time.Hour),
				},
			)
		},
	)
}

func (ss *SQLServer) ListBackups(ctx context.Context, cid string, pageToken bmodels.BackupsPageToken, pageSize int64) ([]bmodels.Backup, bmodels.BackupsPageToken, error) {
	var backups []bmodels.Backup
	var nextPageToken bmodels.BackupsPageToken
	pageSize = pagination.SanePageSize(pageSize)
	if err := ss.operator.ReadOnCluster(ctx, cid, cmodels.TypeSQLServer,
		func(ctx context.Context, session sessions.Session, reader clusters.Reader, _ clusters.Cluster) error {
			var err2 error
			backups, nextPageToken, err2 = ss.backups.BackupsByClusterID(ctx, cid,
				func(ctx context.Context, client s3.Client, cid string) ([]bmodels.Backup, error) {
					return ss.listS3BackupImpl(ctx, reader, client, cid)
				},
				pageToken, optional.NewInt64(pageSize))
			for i := range backups {
				backups[i].FolderID = session.FolderCoords.FolderExtID
			}
			return err2
		},
	); err != nil {
		return []bmodels.Backup{}, bmodels.BackupsPageToken{}, err
	}
	return backups, nextPageToken, nil
}

func (ss *SQLServer) ListBackupsInFolder(ctx context.Context, folderExtID string, pageToken bmodels.BackupsPageToken, pageSize int64) ([]bmodels.Backup, bmodels.BackupsPageToken, error) {
	var backups []bmodels.Backup
	var nextPageToken bmodels.BackupsPageToken
	pageSize = pagination.SanePageSize(pageSize)
	if err := ss.operator.ReadOnFolder(ctx, folderExtID,
		func(ctx context.Context, session sessions.Session, reader clusters.Reader) error {
			var err2 error
			backups, nextPageToken, err2 = ss.backups.BackupsByFolderID(ctx, session.FolderCoords.FolderID,
				cmodels.TypeSQLServer,
				func(ctx context.Context, client s3.Client, cid string) ([]bmodels.Backup, error) {
					return ss.listS3BackupImpl(ctx, reader, client, cid)
				},
				pageToken, pageSize,
			)
			for i := range backups {
				backups[i].FolderID = session.FolderCoords.FolderExtID
			}
			return err2
		},
	); err != nil {
		return []bmodels.Backup{}, bmodels.BackupsPageToken{}, err
	}
	return backups, nextPageToken, nil
}

func (ss *SQLServer) BackupByGlobalID(ctx context.Context, globalBackupID string) (bmodels.Backup, error) {
	cid, backupID, err := bmodels.DecodeGlobalBackupID(globalBackupID)
	if err != nil {
		return bmodels.Backup{}, err
	}

	var backup bmodels.Backup
	if err = ss.operator.ReadOnDeletedCluster(ctx, cid,
		func(ctx context.Context, session sessions.Session, reader clusters.Reader) error {
			backup, err = ss.backups.BackupByClusterIDBackupID(ctx, cid, backupID,
				func(ctx context.Context, client s3.Client, cid string) ([]bmodels.Backup, error) {
					return ss.listS3BackupImpl(ctx, reader, client, cid)
				})
			if err != nil {
				return err
			}

			backup.FolderID = session.FolderCoords.FolderExtID
			return nil
		},
	); err != nil {
		return bmodels.Backup{}, err
	}

	return backup, nil
}
