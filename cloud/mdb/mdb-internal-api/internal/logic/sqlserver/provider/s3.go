package provider

import (
	"context"
	"encoding/json"
	"fmt"
	"path"
	"strings"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/s3"
	clusterslogic "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/sqlserver/provider/internal/sspillars"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/backups"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/library/go/core/xerrors"
)

const (
	SQLServerSentinelSuffix = "_backup_stop_sentinel.json"
)

type clusterReader interface {
	ClusterByClusterID(ctx context.Context, cid string, typ clusters.Type, vis models.Visibility) (clusterslogic.Cluster, error)
}

func (ss *SQLServer) listS3BackupImpl(ctx context.Context, reader clusterReader, client s3.Client, cid string) ([]backups.Backup, error) {
	cluster, err := reader.ClusterByClusterID(ctx, cid, clusters.TypeSQLServer, models.VisibilityVisibleOrDeleted)
	if err != nil {
		return nil, err
	}
	var pillar sspillars.Cluster
	err = cluster.Pillar(&pillar)
	if err != nil {
		return nil, err
	}

	bucket := pillar.Data.S3Bucket
	prefix := fmt.Sprintf("wal-g/%s/%s/basebackups_005/", cid, pillar.Data.SQLServer.Version.Major)
	delimiter := "/"

	objects, _, err := client.ListObjects(ctx, bucket, s3.ListObjectsOpts{Prefix: &prefix, Delimiter: &delimiter})
	if err != nil {
		if xerrors.Is(err, s3.ErrNotFound) {
			ss.L.Warnf("s3 bucket not exist: %+v", bucket)
			return nil, nil
		}
		return nil, err
	}

	var res []backups.Backup
	var databases []string
	for _, obj := range objects {
		if strings.HasSuffix(obj.Key, SQLServerSentinelSuffix) {
			r, err := client.GetObject(ctx, bucket, obj.Key)
			if err != nil {
				return nil, err
			}
			defer r.Close()
			var sentinel struct {
				Databases      []string
				StartLocalTime time.Time
				StopLocalTime  time.Time `json:"StopLocalTime,omitempty"`
			}
			err = json.NewDecoder(r).Decode(&sentinel)
			if err != nil {
				return nil, err
			}
			backupID := strings.TrimSuffix(path.Base(obj.Key), SQLServerSentinelSuffix)
			createdAt := obj.LastModified
			if !sentinel.StopLocalTime.IsZero() {
				createdAt = sentinel.StopLocalTime
			}
			if createdAt.Nanosecond() > 0 {
				createdAt = createdAt.Truncate(time.Second).Add(time.Second)
			}
			if len(sentinel.Databases) != 0 {
				databases = sentinel.Databases
			}
			backup := backups.Backup{
				ID:              backupID,
				SourceClusterID: cluster.ClusterID,
				// FolderID: cluster.  ?? TODO: load folder with cluster ?
				Databases: databases,
				StartedAt: optional.NewTime(sentinel.StartLocalTime),
				CreatedAt: createdAt,
			}
			res = append(res, backup)
		}
	}

	return res, nil
}
