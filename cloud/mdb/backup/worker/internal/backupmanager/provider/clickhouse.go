package provider

import (
	"context"
	"encoding/json"
	"fmt"
	"io"
	"io/ioutil"
	"path"
	"strings"
	"time"

	"a.yandex-team.ru/cloud/mdb/backup/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/backup/worker/internal/backupmanager"
	"a.yandex-team.ru/cloud/mdb/backup/worker/internal/models"
	"a.yandex-team.ru/cloud/mdb/internal/s3"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

const (
	ClusterBackupPathPrefix = "ch_backup/%s/"
	ShardBackupPathPrefix   = ClusterBackupPathPrefix + "%s/"
	BackupPathDelimiter     = "/"
)

var BackupMetaFiles = []string{
	"backup_light_struct.json",
	"backup_struct.json",
}

type BackupLabels struct {
	Name      string `json:"name"`
	ShardName string `json:"shard_name"`
}

type ClickhouseS3BackupMeta struct {
	Meta struct {
		DateFmt   string       `json:"date_fmt"`
		StartTime string       `json:"start_time"`
		EndTime   string       `json:"end_time"`
		Labels    BackupLabels `json:"labels"`
		State     string       `json:"state"`
		Size      int64        `json:"bytes"`
	}
}

type ClickhouseMetadata struct {
	BackupName string          `json:"name,omitempty"`
	BackupID   string          `json:"id,omitempty"`
	StartTime  time.Time       `json:"start_time,omitempty"`
	FinishTime time.Time       `json:"finish_time,omitempty"`
	Size       int64           `json:"size,omitempty"`
	RootPath   string          `json:"root_path,omitempty"`
	ShardNames []string        `json:"shard_names,omitempty"`
	Labels     BackupLabels    `json:"labels,omitempty"`
	RawMeta    json.RawMessage `json:"raw_meta,omitempty"`
}

type ClickhouseS3BackupManager struct {
	s3 s3.Client
	lg log.Logger
}

func NewClickhouseS3DBBackupManager(s3 s3.Client, lg log.Logger) *ClickhouseS3BackupManager {
	return &ClickhouseS3BackupManager{s3: s3, lg: lg}
}

func (cbm *ClickhouseS3BackupManager) ListBackups(ctx context.Context, bucket, path string) ([]metadb.BackupMetadata, error) {
	return listS3ClusterBackupsImpl(ctx, cbm.s3, bucket, path)
}

func listS3ClusterBackupsImpl(ctx context.Context, client s3.Client, bucket, prefix string) ([]metadb.BackupMetadata, error) {

	delimiter := BackupPathDelimiter

	_, folders, err := client.ListObjects(ctx, bucket, s3.ListObjectsOpts{Prefix: &prefix, Delimiter: &delimiter})
	if err != nil {
		if xerrors.Is(err, s3.ErrNotFound) {
			return nil, xerrors.Errorf("s3 bucket not exist: %+v", bucket)
		}
		return nil, err
	}
	var res []metadb.BackupMetadata

	for _, folder := range folders {
		found := false
		for _, metaFile := range BackupMetaFiles {
			stream, err := client.GetObject(ctx, bucket, path.Join(folder.Prefix, metaFile))
			if err == nil {
				backup, err := metadata(stream)
				if err == nil {

					backup.BackupID = path.Base(folder.Prefix)
					backup.RootPath = folder.Prefix

					res = append(res, backup)
					found = true
					break
				}
			}
		}

		if !found {
			rec, err := listS3ClusterBackupsImpl(ctx, client, bucket, folder.Prefix)
			if err == nil {
				res = append(res, rec...)
			}
		}
	}

	return res, nil
}

var _ metadb.BackupMetadata = ClickhouseMetadata{}

func (meta ClickhouseMetadata) ID() string {
	return meta.BackupID
}

func (meta ClickhouseMetadata) Marshal() ([]byte, error) {
	return json.Marshal(meta)
}

func (meta ClickhouseMetadata) String() string {
	raw, err := meta.Marshal()
	if err != nil {
		return fmt.Sprintf("can not marshal clickhouse metadata with error %v: %+v", err, raw)
	}
	return string(raw)
}

type ClickhouseJobFactsExtractor struct {
	mdb metadb.MetaDB
}

func NewClickhouseJobFactsExtractor(mdb metadb.MetaDB) *ClickhouseJobFactsExtractor {
	return &ClickhouseJobFactsExtractor{
		mdb: mdb,
	}
}

func (m *ClickhouseJobFactsExtractor) BackupPathFromJob(ctx context.Context, job models.BackupJob) (string, error) {
	shards, err := m.mdb.ListShards(ctx, job.ClusterID)
	if err != nil {
		return "", err
	}
	jobShardID, err := job.ShardID.Get()
	if err != nil {
		return "", xerrors.Errorf("Could not get job shard id for cluster: %s. With error: %w", job.ClusterID, err)
	}
	for _, shard := range shards {
		shardID, err := shard.ID.Get()
		if err != nil {
			return "", xerrors.Errorf("could not get shard id for cluster: %s. With error: %w", job.ClusterID, err)
		}
		if shardID == jobShardID {
			shardName, err := shard.Name.Get()
			if err != nil {
				return "", xerrors.Errorf("could not get shard name for shard: %s, for cluster: %s. "+
					"With error: %w", shardID, job.ClusterID, err)
			}
			return BackupPathForShard(job.ClusterID, shardName), nil
		}
	}

	return "", xerrors.Errorf("could not found shard with id: %s, for cluster: %s", jobShardID, job.ClusterID)
}

func BackupPathForShard(cid string, shard string) string {
	return fmt.Sprintf(ShardBackupPathPrefix, cid, shard)
}

func BackupPathForCluster(cid string) string {
	return fmt.Sprintf(ClusterBackupPathPrefix, cid)
}

func (m *ClickhouseJobFactsExtractor) BackupNameRootFromJob(ctx context.Context, job models.BackupJob) (string, string, error) {
	meta, err := UnmarshalClickhouseMetadata(job.Metadata)
	if err != nil {
		return "", "", xerrors.Errorf("can not unmarshal clickhouse metadata with error: %w", err)
	}
	return meta.BackupName, meta.RootPath, nil
}

type ClickhouseDeployArgsProvider struct {
	lg log.Logger
}

func NewClickhouseDeployArgsProvider() *ClickhouseDeployArgsProvider {
	return &ClickhouseDeployArgsProvider{}
}

type PillarLabels struct {
	BackupName string `json:"name"`
}

type ClickhouseCreationPillar struct {
	BackupID          string       `json:"backup_id"`
	Labels            PillarLabels `json:"labels,omitempty"`
	BackupName        string       `json:"name,omitempty"`
	BackupIsPermanent bool         `json:"user_backup,omitempty"`
}

type ClickhouseDeletionPillar struct {
	BackupID   string `json:"backup_id"`
	BackupName string `json:"backup_name"`
}

func (chdp *ClickhouseDeployArgsProvider) CreationArgsFromJob(ctx context.Context, job models.BackupJob, deployType backupmanager.DeployType) ([]string, error) {
	if deployType != backupmanager.StateDeployType {
		return nil, xerrors.Errorf("unexpected deploy type")
	}

	pillar := ClickhouseCreationPillar{
		BackupID:          job.BackupID,
		BackupIsPermanent: job.Initiator == metadb.BackupInitiatorUser,
	}

	if len(job.Metadata) > 0 {
		meta, err := UnmarshalClickhouseMetadata(job.Metadata)
		if err != nil {
			return nil, xerrors.Errorf("can not unmarshal clickhouse metadata with error: %w", err)
		}
		if len(meta.BackupName) > 0 {
			pillar.Labels = PillarLabels{
				BackupName: meta.BackupName,
			}
		}
	}

	return backupmanager.PillarToArgs(pillar)
}

func (chdp *ClickhouseDeployArgsProvider) DeletionArgsFromJob(ctx context.Context, job models.BackupJob) ([]string, error) {
	meta, err := UnmarshalClickhouseMetadata(job.Metadata)
	if err != nil {
		return nil, err
	}

	pillar := ClickhouseDeletionPillar{
		BackupID:   job.BackupID,
		BackupName: meta.BackupName,
	}

	return backupmanager.PillarToArgs(pillar)
}

func UnmarshalClickhouseMetadata(raw []byte) (ClickhouseMetadata, error) {
	var meta ClickhouseMetadata
	if err := json.Unmarshal(raw, &meta); err != nil {
		return ClickhouseMetadata{}, err
	}
	return meta, nil
}

func metadata(stream io.ReadCloser) (ClickhouseMetadata, error) {
	raw, err := ioutil.ReadAll(stream)
	if err != nil {
		return ClickhouseMetadata{}, err
	}

	var backupMeta ClickhouseS3BackupMeta
	err = json.Unmarshal(raw, &backupMeta)
	if err != nil {
		return ClickhouseMetadata{}, err
	}

	if backupMeta.Meta.State != "created" {
		return ClickhouseMetadata{}, xerrors.Errorf("backup is not created")
	}
	if len(backupMeta.Meta.DateFmt) == 0 {
		return ClickhouseMetadata{}, xerrors.Errorf("unexpected backup meta")
	}

	var createdAt, startedAt time.Time
	startedAt, err = backupTime(backupMeta.Meta.StartTime, backupMeta.Meta.DateFmt)
	if len(backupMeta.Meta.EndTime) != 0 {
		createdAt, err = backupTime(backupMeta.Meta.EndTime, backupMeta.Meta.DateFmt)
	}
	if err != nil {
		return ClickhouseMetadata{}, err
	}

	var shardNames []string
	if len(backupMeta.Meta.Labels.ShardName) != 0 {
		shardNames = []string{backupMeta.Meta.Labels.ShardName}
	}

	return ClickhouseMetadata{
		BackupName: backupMeta.Meta.Labels.Name,
		StartTime:  startedAt,
		FinishTime: createdAt,
		Size:       backupMeta.Meta.Size,
		ShardNames: shardNames,
		Labels:     backupMeta.Meta.Labels,
		RawMeta:    raw,
	}, nil
}

func backupTime(t, dateFormat string) (time.Time, error) {
	return time.Parse(cDateNotationToGoDateLayout(dateFormat), t)
}

func cDateNotationToGoDateLayout(cLayout string) string {
	replacer := strings.NewReplacer(
		"%a", "Mon",
		"%A", "Monday",
		"%b", "Jan",
		"%B", "January",
		"%c", "Mon Jan 02 15:04:05 2006",
		"%d", "02",
		"%D", "01/02/06",
		"%e", "2",
		"%F", "2006-01-02",
		"%h", "Jan",
		"%H", "15",
		"%I", "3",
		"%m", "01",
		"%M", "04",
		"%n", "\n",
		"%p", "PM",
		"%r", "03:04:05 PM",
		"%R", "15:04",
		"%S", "05",
		"%t", "\t",
		"%T", "15:04:05",
		"%x", "01/02/06",
		"%X", "15:04:05",
		"%y", "06",
		"%Y", "2006",
		"%z", "-0700",
		"%Z", "MST",
	)
	return replacer.Replace(cLayout)
}
