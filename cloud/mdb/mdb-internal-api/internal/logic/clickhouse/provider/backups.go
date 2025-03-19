package provider

import (
	"context"
	"encoding/json"
	"fmt"
	"io"
	"math"
	"path"
	"time"

	cheventspub "a.yandex-team.ru/cloud/bitbucket/public-api/yandex/cloud/events/mdb/clickhouse"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/s3"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/clickhouse/chmodels"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/clickhouse/provider/internal/chpillars"
	clusterslogic "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/sessions"
	taskslogic "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/tasks"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	bmodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/backups"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/hosts"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/pagination"
	tasksmodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/tasks"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/slices"
)

func (ch *ClickHouse) BackupCluster(ctx context.Context, cid string, name optional.String) (operations.Operation, error) {

	return ch.operator.ModifyOnNotStoppedCluster(ctx, cid, clusters.TypeClickHouse,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, modifier clusterslogic.Modifier, cluster clusterslogic.Cluster) (operations.Operation, error) {

			chSubCluster, err := ch.chSubCluster(ctx, reader, cluster.ClusterID)
			if err != nil {
				return operations.Operation{}, err
			}
			useBackupAPI, err := reader.ClusterUsesBackupService(ctx, cid)
			if err != nil {
				return operations.Operation{}, err
			}
			var op operations.Operation
			if name.Valid {
				if err := chmodels.ValidateBackupName(name.String); err != nil {
					return operations.Operation{}, err
				}
			}
			if useBackupAPI {
				shards, err2 := reader.ListShards(ctx, cid)
				if err2 != nil {
					return operations.Operation{}, err2
				}
				op, err = ch.doBackupServiceBackup(ctx, session, cluster, chSubCluster, name, shards)
			} else {
				allHosts, err2 := clusterslogic.ListAllHosts(ctx, reader, cid)
				if err2 != nil {
					return operations.Operation{}, err2
				}

				chHosts := clusterslogic.GetHostsWithRole(allHosts, hosts.RoleClickHouse)
				hostsByShard := chmodels.SplitHostsExtendedByShard(chHosts)
				var maxDisk int64 = 0
				for _, hg := range hostsByShard {
					if hg[0].SpaceLimit > maxDisk {
						maxDisk = hg[0].SpaceLimit
					}
				}
				op, err = ch.doRegularBackup(ctx, session, cluster, chSubCluster, name,
					backupTimeoutBasedOnDisk(maxDisk, ch.cfg.ClickHouse.Backup.MinimalBackupTimeLimitHours),
				)
			}

			if err != nil {
				return operations.Operation{}, err
			}

			event := &cheventspub.BackupCluster{
				Authentication:  ch.events.NewAuthentication(session.Subject),
				Authorization:   ch.events.NewAuthorization(session.Subject),
				RequestMetadata: ch.events.NewRequestMetadata(ctx),
				EventStatus:     cheventspub.BackupCluster_STARTED,
				Details: &cheventspub.BackupCluster_EventDetails{
					ClusterId: cluster.ClusterID,
				},
			}
			em, err := ch.events.NewEventMetadata(event, op, session.FolderCoords.CloudExtID, session.FolderCoords.FolderExtID)
			if err != nil {
				return operations.Operation{}, err
			}
			event.EventMetadata = em

			if err = ch.events.Store(ctx, event, op); err != nil {
				return operations.Operation{}, xerrors.Errorf("failed to save event for op %+v: %w", op, err)
			}

			return op, nil
		})

}

func (ch *ClickHouse) doBackupServiceBackup(ctx context.Context, session sessions.Session, cluster clusterslogic.Cluster, chSubCluster subCluster, backupName optional.String, shards []clusters.Shard) (operations.Operation, error) {
	var backupIDs []string
	for _, shard := range shards {
		metadata := BackupMetadata{}
		if backupName.Valid {
			metadata.BackupName = backupName.String
		}

		backupID, err := ch.backups.ScheduleBackupForNow(ctx, cluster.ClusterID, chSubCluster.SubClusterID, shard.ShardID, bmodels.BackupMethodFull, metadata)
		if err != nil {
			return operations.Operation{}, err
		}
		backupIDs = append(backupIDs, backupID)
	}

	return ch.tasks.CreateTask(
		ctx,
		session,
		tasksmodels.CreateTaskArgs{
			ClusterID:     cluster.ClusterID,
			FolderID:      session.FolderCoords.FolderID,
			Auth:          session.Subject,
			TaskType:      chmodels.TaskTypeClusterWaitBackupService,
			OperationType: chmodels.OperationTypeClusterBackup,
			TaskArgs: map[string]interface{}{
				"backup_ids": backupIDs,
			},
			Metadata: chmodels.MetadataBackupCluster{},
			Revision: cluster.Revision,
		})

}

func (ch *ClickHouse) doRegularBackup(ctx context.Context, session sessions.Session, cluster clusterslogic.Cluster, chSubCluster subCluster, backupName optional.String, backupTimeout time.Duration) (operations.Operation, error) {
	if chSubCluster.Pillar.CloudStorageEnabled() {
		versionGreater, err := chmodels.VersionGreaterOrEqual(chSubCluster.Pillar.Data.ClickHouse.Version, 21, 6)
		if err != nil {
			return operations.Operation{}, semerr.FailedPrecondition(err.Error())
		}
		if !versionGreater {
			return operations.Operation{}, semerr.FailedPrecondition("cloud storage backups are not supported")
		}
	}

	taskArgs := map[string]interface{}{
		"time_limit": backupTimeout,
	}

	if backupName.Valid {
		taskArgs["labels"] = map[string]string{
			"name": backupName.String,
		}
	}

	return ch.tasks.BackupCluster(
		ctx,
		session,
		cluster.ClusterID,
		cluster.Revision,
		chmodels.TaskTypeClusterBackup,
		chmodels.OperationTypeClusterBackup,
		chmodels.MetadataBackupCluster{},
		taskslogic.BackupClusterTaskArgs(taskArgs),
	)
}

func (ch *ClickHouse) Backup(ctx context.Context, globalBackupID string) (bmodels.Backup, error) {
	cid, backupID, err := bmodels.DecodeGlobalBackupID(globalBackupID)
	if err != nil {
		return bmodels.Backup{}, err
	}

	var backup bmodels.Backup

	if err := ch.operator.ReadOnDeletedCluster(ctx, cid,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader) error {
			useBackupAPI, err := reader.ClusterUsesBackupService(ctx, cid)
			if err != nil {
				return err
			}

			if useBackupAPI {
				backup, err = ch.backups.ManagedBackupByBackupID(ctx, backupID, BackupConverter{})
			} else {
				backup, err = ch.backups.BackupByClusterIDBackupID(ctx, cid, backupID,
					func(ctx context.Context, client s3.Client, cid string) ([]bmodels.Backup, error) {
						return listS3ClusterBackups(ctx, reader, client, cid, backupID)
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

func (ch *ClickHouse) FolderBackups(ctx context.Context, fid string, pageToken bmodels.BackupsPageToken, pageSize int64) ([]bmodels.Backup, bmodels.BackupsPageToken, error) {
	var (
		backups       []bmodels.Backup
		nextPageToken bmodels.BackupsPageToken
		err           error
	)

	err = ch.operator.ReadOnFolder(ctx, fid,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader) error {
			backups, nextPageToken, err = ch.backups.BackupsByFolderID(ctx, session.FolderCoords.FolderID, clusters.TypeClickHouse,
				func(ctx context.Context, client s3.Client, cid string) ([]bmodels.Backup, error) {

					useBackupAPI, err := reader.ClusterUsesBackupService(ctx, cid)
					if err != nil {
						return nil, err
					}
					if useBackupAPI {
						return ch.backups.ManagedBackupsByClusterID(ctx, cid, []bmodels.BackupStatus{bmodels.BackupStatusDone}, BackupConverter{})
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

func (ch *ClickHouse) ClusterBackups(ctx context.Context, cid string, pageToken bmodels.BackupsPageToken, pageSize int64) ([]bmodels.Backup, bmodels.BackupsPageToken, error) {
	var backups []bmodels.Backup
	var nextPageToken bmodels.BackupsPageToken
	pageSize = pagination.SanePageSize(pageSize)

	var err error
	if err = ch.operator.ReadOnCluster(ctx, cid, clusters.TypeClickHouse,
		func(ctx context.Context, session sessions.Session, reader clusterslogic.Reader, cluster clusterslogic.Cluster) error {
			useBackupAPI, err := reader.ClusterUsesBackupService(ctx, cid)
			if err != nil {
				return err
			}

			pageSize := optional.NewInt64(pageSize)

			backups, nextPageToken, err = ch.backups.BackupsByClusterID(ctx, cluster.ClusterID,
				func(ctx context.Context, client s3.Client, cid string) ([]bmodels.Backup, error) {
					if useBackupAPI {
						return ch.backups.ManagedBackupsByClusterID(ctx, cid, []bmodels.BackupStatus{bmodels.BackupStatusDone}, BackupConverter{})
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

func listS3ClusterBackups(ctx context.Context, reader clusterslogic.Reader, client s3.Client, cid string, targetBackups ...string) ([]bmodels.Backup, error) {
	cluster, err := reader.ClusterByClusterID(ctx, cid, clusters.TypeClickHouse, models.VisibilityVisibleOrDeleted)
	if err != nil {
		return nil, err
	}
	var pillar chpillars.ClusterCH
	err = cluster.Pillar(&pillar)
	if err != nil {
		return nil, err
	}

	bucket := pillar.Data.S3Bucket
	prefix := fmt.Sprintf(chmodels.BackupPathPrefix, cid)

	_, shards, err := client.ListObjects(ctx, bucket, s3.ListObjectsOpts{Prefix: &prefix, Delimiter: &chmodels.BackupPathDelimiter})
	if err != nil {
		if xerrors.Is(err, s3.ErrNotFound) {
			return nil, xerrors.Errorf("s3 bucket not exist: %+v", bucket)
		}
		return nil, err
	}

	shards = append(shards, s3.Prefix{Prefix: prefix}) // Handle legacy layout

	result := make([]bmodels.Backup, 0, len(targetBackups))
	for _, shard := range shards {
		shardBackups, err := listS3ClusterBackupsImpl(ctx, client, bucket, cid, shard.Prefix, targetBackups...)
		if err != nil {
			return nil, err
		}

		result = append(result, shardBackups...)
		if targetBackups != nil {
			if len(result) == len(targetBackups) {
				return result, nil
			}
		}
	}

	if targetBackups != nil && len(targetBackups) != len(result) {
		found := map[string]struct{}{}

		for _, backup := range result {
			found[backup.ID] = struct{}{}
		}

		for _, backup := range targetBackups {
			if _, ok := found[backup]; !ok {
				return nil, semerr.NotFoundf("backup %q does not exist", bmodels.EncodeGlobalBackupID(cid, backup))
			}
		}
	}

	return result, nil
}

func listS3ClusterBackupsImpl(ctx context.Context, client s3.Client, bucket, cid, prefix string, targetBackups ...string) ([]bmodels.Backup, error) {

	_, folders, err := client.ListObjects(ctx, bucket, s3.ListObjectsOpts{Prefix: &prefix, Delimiter: &chmodels.BackupPathDelimiter})
	if err != nil {
		if xerrors.Is(err, s3.ErrNotFound) {
			return nil, xerrors.Errorf("s3 bucket not exist: %+v", bucket)
		}
		return nil, err
	}

	var res []bmodels.Backup

	for _, folder := range folders {
		if len(targetBackups) > 0 && !slices.ContainsString(targetBackups, path.Base(folder.Prefix)) {
			continue
		}

		for _, metaFile := range chmodels.BackupMetaFiles {
			stream, err := client.GetObject(ctx, bucket, path.Join(folder.Prefix, metaFile))
			if err == nil {
				backup, err := backupFromS3(stream)
				if err == nil {
					backup.ID = path.Base(folder.Prefix)
					backup.SourceClusterID = cid
					backup.S3Path = folder.Prefix
					res = append(res, backup)
					break
				}
			}
		}
	}

	return res, nil
}

func backupFromS3(stream io.ReadCloser) (bmodels.Backup, error) {
	var backupMeta chmodels.S3BackupMeta
	err := json.NewDecoder(stream).Decode(&backupMeta)
	if err != nil {
		return bmodels.Backup{}, err
	}

	backup, err := chmodels.BackupFromBackupMeta(backupMeta)
	if err != nil {
		return bmodels.Backup{}, err
	}

	return backup, stream.Close()
}

func backupTimeoutBasedOnDisk(diskSize int64, minimalTimeLimit int) time.Duration {
	// expect 1 GB to be transferred in 60 seconds
	res := int(math.Ceil(float64(diskSize) / math.Pow(1024, 3) / 3600))

	if res < minimalTimeLimit {
		res = minimalTimeLimit
	}

	return time.Hour * time.Duration(res)
}

type BackupMetadata struct {
	BackupName string                `json:"name"`
	ShardNames []string              `json:"shard_names"`
	RootPath   string                `json:"root_path"`
	Size       int64                 `json:"size"`
	Labels     chmodels.BackupLabels `json:"labels"`
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
		b.Metadata = metadata.Labels
	}

	return b, nil
}
