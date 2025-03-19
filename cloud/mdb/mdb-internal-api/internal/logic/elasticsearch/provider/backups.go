package provider

import (
	"context"
	"encoding/json"
	"errors"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/s3"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/elasticsearch/esmodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/elasticsearch/provider/internal/espillars"
	clusterslogic "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/sessions"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	bmodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/backups"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/pagination"
)

func backupMetaFromS3(ctx context.Context, client s3.Client, bucket, key string) (esmodels.BackupMeta, error) {
	r, err := client.GetObject(ctx, bucket, key)
	if err != nil {
		return esmodels.BackupMeta{}, err
	}
	defer r.Close()

	var backupMeta esmodels.BackupMeta
	err = json.NewDecoder(r).Decode(&backupMeta)
	if err != nil {
		return esmodels.BackupMeta{}, err
	}
	return backupMeta, nil
}

func (es *ElasticSearch) listClusterBackupsImpl(ctx context.Context, reader clusterslogic.Reader, client s3.Client, cid string) ([]bmodels.Backup, error) {
	cluster, err := reader.ClusterByClusterID(ctx, cid, clusters.TypeElasticSearch, models.VisibilityVisibleOrDeleted)
	if err != nil {
		return nil, err
	}
	var pillar espillars.Cluster
	err = cluster.Pillar(&pillar)
	if err != nil {
		return nil, err
	}

	bucket := pillar.Data.S3Bucket
	if bucket == "" {
		bucket = pillar.Data.S3Buckets["backup"]
	}
	if bucket == "" {
		bucket = pillar.Data.S3Buckets["secure_backups"]
	}

	metadata, err := backupMetaFromS3(ctx, client, bucket, esmodels.S3MetaKey)
	if err != nil && !errors.Is(err, s3.ErrNotFound) {
		return nil, err
	}
	var res []bmodels.Backup

	for _, s := range metadata.Snapshots {
		res = append(res, bmodels.Backup{
			ID:              s.ID,
			SourceClusterID: cluster.ClusterID,
			CreatedAt:       time.Unix(s.EndTimeMs/1000, (s.EndTimeMs%1000)*int64(time.Millisecond)).UTC(),
			StartedAt:       optional.NewTime(time.Unix(s.StartTimeMs/1000, (s.StartTimeMs%1000)*int64(time.Millisecond)).UTC()),
			Metadata:        s,
		})
	}

	return res, nil
}

func (es *ElasticSearch) Backup(ctx context.Context, globalBackupID string) (bmodels.Backup, error) {
	cid, backupID, err := bmodels.DecodeGlobalBackupID(globalBackupID)
	if err != nil {
		return bmodels.Backup{}, err
	}

	var backup bmodels.Backup

	err = es.operator.ReadOnDeletedCluster(ctx, cid,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader) error {
			if backup, err = es.backups.BackupByClusterIDBackupID(ctx, cid, backupID,
				func(ctx context.Context, client s3.Client, cid string) ([]bmodels.Backup, error) {
					return es.listClusterBackupsImpl(ctx, reader, client, cid)
				},
			); err != nil {
				return err
			}

			backup.FolderID = session.FolderCoords.FolderExtID

			return nil
		},
		clusterslogic.WithPermission(esmodels.PermClustersGet),
	)
	if err != nil {
		return bmodels.Backup{}, err
	}

	return backup, nil
}

func (es *ElasticSearch) FolderBackups(ctx context.Context, fid string, pageToken bmodels.BackupsPageToken, pageSize int64) ([]bmodels.Backup, bmodels.BackupsPageToken, error) {
	var backups []bmodels.Backup
	var nextPageToken bmodels.BackupsPageToken
	pageSize = pagination.SanePageSize(pageSize)

	err := es.operator.ReadOnFolder(ctx, fid,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader) error {
			var err2 error
			backups, nextPageToken, err2 = es.backups.BackupsByFolderID(ctx, session.FolderCoords.FolderID, clusters.TypeElasticSearch,
				func(ctx context.Context, client s3.Client, cid string) ([]bmodels.Backup, error) {
					return es.listClusterBackupsImpl(ctx, reader, client, cid)
				},
				pageToken, pageSize)
			for i := range backups {
				backups[i].FolderID = session.FolderCoords.FolderExtID
			}
			return err2
		},
		clusterslogic.WithPermission(esmodels.PermClustersGet),
	)
	if err != nil {
		return nil, nextPageToken, err
	}

	return backups, nextPageToken, nil
}

func (es *ElasticSearch) ClusterBackups(ctx context.Context, cid string, pageToken bmodels.BackupsPageToken, pageSize int64) ([]bmodels.Backup, bmodels.BackupsPageToken, error) {
	var backups []bmodels.Backup
	var nextPageToken bmodels.BackupsPageToken
	pageSize = pagination.SanePageSize(pageSize)

	var err = es.operator.ReadOnCluster(ctx, cid, clusters.TypeElasticSearch,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, cluster clusterslogic.Cluster) error {
			var err2 error
			backups, nextPageToken, err2 = es.backups.BackupsByClusterID(ctx, cluster.ClusterID,
				func(ctx context.Context, client s3.Client, cid string) ([]bmodels.Backup, error) {
					return es.listClusterBackupsImpl(ctx, reader, client, cid)
				},
				pageToken, optional.NewInt64(pageSize))

			for i := range backups {
				backups[i].FolderID = session.FolderCoords.FolderExtID
			}

			return err2
		},
		clusterslogic.WithPermission(esmodels.PermClustersGet),
	)
	if err != nil {
		return nil, nextPageToken, err
	}

	return backups, nextPageToken, nil
}
