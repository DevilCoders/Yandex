package provider

import (
	"context"
	"encoding/json"
	"io"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/s3"
	clusterslogic "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/sessions"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/redis/provider/internal/rpillars"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/redis/rmodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	bmodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/backups"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/pagination"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (r *Redis) Backup(ctx context.Context, globalBackupID string) (bmodels.Backup, error) {
	cid, backupID, err := rmodels.DecodeGlobalBackupID(globalBackupID)
	if err != nil {
		return bmodels.Backup{}, err
	}

	var backup bmodels.Backup

	if err := r.operator.ReadOnDeletedCluster(ctx, cid,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader) error {
			useBackupAPI, err := reader.ClusterUsesBackupService(ctx, cid)
			if err != nil {
				return err
			}

			if useBackupAPI {
				backup, err = r.backups.ManagedBackupByBackupID(ctx, backupID, BackupConverter{})
			} else {
				backup, err = r.backups.BackupByClusterIDBackupID(ctx, cid, backupID,
					func(ctx context.Context, client s3.Client, cid string) ([]bmodels.Backup, error) {
						return listS3ClusterBackups(ctx, reader, client, cid)
					},
				)
			}
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

func listS3ClusterBackups(ctx context.Context, reader clusterslogic.Reader, client s3.Client, cid string) ([]bmodels.Backup, error) {
	cluster, err := reader.ClusterByClusterID(ctx, cid, clusters.TypeRedis, models.VisibilityVisibleOrDeleted)
	if err != nil {
		return nil, err
	}
	var pillar rpillars.Cluster
	err = cluster.Pillar(&pillar)
	if err != nil {
		return nil, err
	}

	bucket := pillar.Data.S3Bucket
	cidPrefix := rmodels.GetBackupCidPrefix(cid)

	return listS3ClusterBackupsImpl(ctx, client, bucket, cidPrefix)
}

func listS3ClusterBackupsImpl(ctx context.Context, client s3.Client, bucket, cidPrefix string) ([]bmodels.Backup, error) {

	delimiter := rmodels.BackupPathDelimiter

	_, shards, err := client.ListObjects(ctx, bucket, s3.ListObjectsOpts{Prefix: &cidPrefix, Delimiter: &delimiter})
	if err != nil {
		if xerrors.Is(err, s3.ErrNotFound) {
			return nil, xerrors.Errorf("s3 bucket not exist: %+v", bucket)
		}
		return nil, err
	}

	var res []bmodels.Backup

	if len(shards) == 0 {
		return res, nil
	}

	for _, shard := range shards {
		backups, err := getBackupsForShard(ctx, client, bucket, delimiter, shard.Prefix)
		if err != nil {
			return nil, err
		}
		res = append(res, backups...)
	}
	return res, nil
}

func getBackupsForShard(ctx context.Context, client s3.Client, bucket, delimiter, prefix string) ([]bmodels.Backup, error) {
	var res []bmodels.Backup

	shardPrefix := rmodels.GetBackupShardPrefix(prefix)

	backups, _, err := client.ListObjects(ctx, bucket, s3.ListObjectsOpts{Prefix: &shardPrefix, Delimiter: &delimiter})
	if err != nil {
		return nil, err
	}
	for _, backup := range backups {
		if !rmodels.IsWalgSentinelFilename(backup.Key) {
			continue
		}
		stream, err := client.GetObject(ctx, bucket, backup.Key)
		if err == nil {
			backup, err := backupFromS3(stream, backup.Key)
			if err == nil {
				res = append(res, backup)
			}
		}
	}
	return res, nil
}

type BackupMetadata struct {
	ShardNames []string `json:"shard_names"`
	RootPath   string   `json:"root_path"`
	Size       int64    `json:"size"`
}

type BackupConverter struct{}

func (bc BackupConverter) ManagedToRegular(mb bmodels.ManagedBackup) (bmodels.Backup, error) {
	var metadata BackupMetadata

	b := bmodels.Backup{
		ID:              mb.ID,
		CreatedAt:       mb.CreatedAt,
		StartedAt:       optional.Time{Time: mb.StartedAt.Time, Valid: mb.StartedAt.Valid},
		SourceClusterID: mb.ClusterID,
	}

	if mb.Metadata.Valid {
		err := json.Unmarshal([]byte(mb.Metadata.String), &metadata)
		if err != nil {
			return bmodels.Backup{}, err
		}
		b.Size = metadata.Size
		b.S3Path = metadata.RootPath
		b.SourceShardNames = metadata.ShardNames
	}

	return b, nil
}

func backupFromS3(stream io.ReadCloser, key string) (bmodels.Backup, error) {
	var backupMeta rmodels.BackupMeta

	err := json.NewDecoder(stream).Decode(&backupMeta)
	if err != nil {
		return bmodels.Backup{}, err
	}

	sd := rmodels.ShardData{}
	err = sd.GetBackupShardData(key)
	if err != nil {
		return bmodels.Backup{}, err
	}
	backup, err := rmodels.BackupFromBackupMeta(backupMeta, sd)
	if err != nil {
		return bmodels.Backup{}, err
	}

	return backup, stream.Close()
}

func (r *Redis) FolderBackups(ctx context.Context, fid string, pageToken bmodels.BackupsPageToken, pageSize int64) ([]bmodels.Backup, bmodels.BackupsPageToken, error) {
	var (
		backups       []bmodels.Backup
		nextPageToken bmodels.BackupsPageToken
		err           error
	)

	err = r.operator.ReadOnFolder(ctx, fid,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader) error {
			backups, nextPageToken, err = r.backups.BackupsByFolderID(ctx, session.FolderCoords.FolderID, clusters.TypeRedis,
				func(ctx context.Context, client s3.Client, cid string) ([]bmodels.Backup, error) {

					useBackupAPI, err := reader.ClusterUsesBackupService(ctx, cid)
					if err != nil {
						return nil, err
					}
					if useBackupAPI {
						return r.backups.ManagedBackupsByClusterID(ctx, cid, []bmodels.BackupStatus{bmodels.BackupStatusDone}, BackupConverter{})
					} else {
						return listS3ClusterBackups(ctx, reader, client, cid)
					}
				},
				pageToken,
				pagination.SanePageSize(pageSize),
			)

			for i := range backups {
				backups[i].FolderID = session.FolderCoords.FolderExtID
			}
			return err
		},
	)
	if err != nil {
		return []bmodels.Backup{}, nextPageToken, err
	}

	return backups, nextPageToken, nil
}

func (r *Redis) ClusterBackups(ctx context.Context, cid string, pageToken bmodels.BackupsPageToken, pageSize int64) ([]bmodels.Backup, bmodels.BackupsPageToken, error) {
	var backups []bmodels.Backup
	var nextPageToken bmodels.BackupsPageToken
	pageSize = pagination.SanePageSize(pageSize)

	var err error
	if err = r.operator.ReadOnCluster(ctx, cid, clusters.TypeRedis,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, cluster clusterslogic.Cluster) error {
			useBackupAPI, err := reader.ClusterUsesBackupService(ctx, cid)
			if err != nil {
				return err
			}

			pageSize := optional.NewInt64(pageSize)

			backups, nextPageToken, err = r.backups.BackupsByClusterID(ctx, cluster.ClusterID,
				func(ctx context.Context, client s3.Client, cid string) ([]bmodels.Backup, error) {
					if useBackupAPI {
						return r.backups.ManagedBackupsByClusterID(ctx, cid, []bmodels.BackupStatus{bmodels.BackupStatusDone}, BackupConverter{})
					} else {
						return listS3ClusterBackups(ctx, reader, client, cid)
					}
				},
				pageToken, pageSize)

			for i := range backups {
				backups[i].FolderID = session.FolderCoords.FolderExtID
			}

			return err
		}); err != nil {
		return []bmodels.Backup{}, nextPageToken, nil
	}

	return backups, nextPageToken, nil
}
