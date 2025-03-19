package provider

import (
	"context"
	"encoding/json"
	"io"
	"strings"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/s3"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/greenplum/gpmodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/greenplum/provider/internal/gppillars"
	clusterslogic "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/sessions"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	bmodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/backups"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/pagination"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func backupFromS3(stream io.ReadCloser) (bmodels.Backup, error) {
	var backupMeta gpmodels.BackupMeta
	err := json.NewDecoder(stream).Decode(&backupMeta)
	if err != nil {
		return bmodels.Backup{}, err
	}

	backup := gpmodels.BackupFromBackupMeta(backupMeta)

	return backup, stream.Close()
}

type BackupMetadata struct {
	CompressedSize int64 `json:"compressed_size"`
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
		b.Size = metadata.CompressedSize

	}
	return b, nil
}

func useBackupAPI(ctx context.Context, reader clusterslogic.Reader, cid string) (bool, error) {
	return reader.ClusterUsesBackupService(ctx, cid)
}

func listClusterBackupsS3(ctx context.Context, reader clusterslogic.Reader, client s3.Client, cid, version string) ([]bmodels.Backup, error) {
	cluster, err := reader.ClusterByClusterID(ctx, cid, clusters.TypeGreenplumCluster, models.VisibilityVisibleOrDeleted)
	if err != nil {
		return []bmodels.Backup{}, err
	}
	var pillar gppillars.Cluster
	err = cluster.Pillar(&pillar)
	if err != nil {
		return []bmodels.Backup{}, err
	}

	bucket := pillar.Data.S3Bucket
	prefix := gpmodels.CalculateBackupPrefix(cid, version)

	return listClusterBackupsImpl(ctx, client, bucket, cid, prefix)
}

func (gp *Greenplum) listClusterBackupsBackupService(ctx context.Context, reader clusterslogic.Reader, client s3.Client, cid string) ([]bmodels.Backup, error) {
	return gp.backups.ManagedBackupsByClusterID(ctx, cid, []bmodels.BackupStatus{bmodels.BackupStatusDone}, BackupConverter{})
}

func (gp *Greenplum) listClusterBackups(ctx context.Context, reader clusterslogic.Reader, client s3.Client, cid string) ([]bmodels.Backup, error) {
	useBackupAPI, err := useBackupAPI(ctx, reader, cid)
	if err != nil {
		return nil, err
	}
	if useBackupAPI {
		return gp.listClusterBackupsBackupService(ctx, reader, client, cid)
	}

	version, err := getMajorVersionPrefix(ctx, reader, cid)
	if err != nil {
		return []bmodels.Backup{}, err
	}

	return listClusterBackupsS3(ctx, reader, client, cid, version)
}

func listClusterBackupsImpl(ctx context.Context, client s3.Client, bucket, cid, prefix string) ([]bmodels.Backup, error) {
	delimiter := gpmodels.BackupPathDelimiter

	objects, _, err := client.ListObjects(ctx, bucket, s3.ListObjectsOpts{Prefix: &prefix, Delimiter: &delimiter})
	if err != nil {
		if xerrors.Is(err, s3.ErrNotFound) {
			return nil, xerrors.Errorf("s3 bucket not exist: %+v", bucket)
		}
		return nil, err
	}

	var res []bmodels.Backup
	suffix := gpmodels.BackupMetaFileSuffix

	for _, object := range objects {
		if !strings.HasSuffix(object.Key, suffix) {
			continue
		}

		stream, err := client.GetObject(ctx, bucket, object.Key)
		if err == nil {
			backup, err := backupFromS3(stream)
			if err == nil {
				backup.SourceClusterID = cid
				res = append(res, backup)
			}
		}
	}

	return res, nil
}

func getMajorVersionPrefix(ctx context.Context, reader clusterslogic.Reader, cid string) (string, error) {
	// returns '6' for '6.17' version
	majorVersion, err := getMajorVersion(ctx, reader, cid)
	if err != nil {
		return "", err
	}
	versionParts := strings.Split(majorVersion, ".")
	prefix := majorVersion
	if len(versionParts) > 1 {
		prefix = versionParts[0]
	}
	return prefix, nil
}

func (gp *Greenplum) Backup(ctx context.Context, globalBackupID string) (bmodels.Backup, error) {
	cid, backupID, err := bmodels.DecodeGlobalBackupID(globalBackupID)
	if err != nil {
		return bmodels.Backup{}, err
	}

	var backup bmodels.Backup

	if err := gp.operator.ReadOnDeletedCluster(ctx, cid,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader) error {
			backup, err = gp.backups.BackupByClusterIDBackupID(ctx, cid, backupID,
				func(ctx context.Context, client s3.Client, cid string) ([]bmodels.Backup, error) {
					return gp.listClusterBackups(ctx, reader, client, cid)
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

func (gp *Greenplum) FolderBackups(ctx context.Context, fid string, pageToken bmodels.BackupsPageToken, pageSize int64) ([]bmodels.Backup, bmodels.BackupsPageToken, error) {
	var (
		backups       []bmodels.Backup
		nextPageToken bmodels.BackupsPageToken
		err           error
	)

	err = gp.operator.ReadOnFolder(ctx, fid,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader) error {
			backups, nextPageToken, err = gp.backups.BackupsByFolderID(ctx, session.FolderCoords.FolderID, clusters.TypeGreenplumCluster,
				func(ctx context.Context, client s3.Client, cid string) ([]bmodels.Backup, error) {
					return gp.listClusterBackups(ctx, reader, client, cid)
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
