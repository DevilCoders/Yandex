package provider

import (
	"context"
	"encoding/json"
	"fmt"
	"io/ioutil"
	"regexp"
	"strings"
	"time"

	"a.yandex-team.ru/cloud/mdb/backup/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/backup/internal/tools/walg"
	"a.yandex-team.ru/cloud/mdb/backup/worker/internal/backupmanager"
	"a.yandex-team.ru/cloud/mdb/backup/worker/internal/models"
	"a.yandex-team.ru/cloud/mdb/internal/s3"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/ptr"
	"a.yandex-team.ru/library/go/x/math"
)

const (
	WalgMongocfgReplSetName   = "mongocfg_subcluster"
	WalgMongoinfraReplSetName = "mongoinfra_subcluster"
)

var (
	walgRootRegexp             = regexp.MustCompile(`^mongodb-backup\/(?P<cid>[^\/]+)\/(?P<replset_id>[^\/]+)`)
	walgRootRegexpCidIndex     = walgRootRegexp.SubexpIndex("cid")
	walgRootRegexpReplsetIndex = walgRootRegexp.SubexpIndex("replset_id")
	walgRootRegexpMaxIndex     = math.MaxInt(walgRootRegexpCidIndex, walgRootRegexpReplsetIndex)
)

func MongodbIDsFromRoot(path string) (cid string, subcid string, err error) {
	res := walgRootRegexp.FindStringSubmatch(path)
	if len(res) != walgRootRegexpMaxIndex+1 {
		return "", "", xerrors.Errorf("can not extract cid/subcid from root %+v: %q", res, path)
	}
	return res[walgRootRegexpCidIndex], res[walgRootRegexpReplsetIndex], nil
}
func MongodbBackupPathFromReplset(clusterID, replsetID string) string {
	return fmt.Sprintf("mongodb-backup/%s/%s/basebackups_005/", clusterID, replsetID)
}

func MongodbClusterBackupDataRoot(root, backup string) string {
	return fmt.Sprintf("%s/basebackups_005/%s/", root, backup)
}

func MongodbClusterBackupRoot(clusterID string) string {
	return fmt.Sprintf("mongodb-backup/%s/", clusterID)
}

type MongodbS3BackupManager struct {
	s3 s3.Client
	lg log.Logger
}

func NewMongoDBBackupManager(s3 s3.Client, lg log.Logger) *MongodbS3BackupManager {
	return &MongodbS3BackupManager{s3: s3, lg: lg}
}

func (mbm *MongodbS3BackupManager) ListBackups(ctx context.Context, bucket, path string) ([]metadb.BackupMetadata, error) {
	objects, _, err := mbm.s3.ListObjects(ctx, bucket, s3.ListObjectsOpts{Prefix: ptr.String(path)})
	if err != nil {
		return nil, err
	}
	backups := make([]metadb.BackupMetadata, 0, len(objects))

	for _, obj := range objects {
		if !strings.HasSuffix(obj.Key, walg.WalgSentinelSuffix) {
			continue
		}

		backup, err := mbm.metadataFromS3Object(ctx, bucket, obj)
		if err != nil {
			return nil, err
		}
		dataSize, err := PrefixSize(ctx, mbm.s3, bucket, MongodbClusterBackupDataRoot(backup.RootPath, backup.BackupName))
		if err != nil {
			return nil, err
		}
		backup.DataSize = dataSize

		backups = append(backups, backup)
	}

	return backups, nil
}

func PrefixSize(ctx context.Context, folder s3.Client, bucket, path string) (int64, error) {
	dataObjects, _, err := folder.ListObjects(ctx, bucket, s3.ListObjectsOpts{Prefix: ptr.String(path)})
	if err != nil {
		return 0, err
	}
	var size int64
	for _, obj := range dataObjects {
		size += obj.Size
	}
	return size, nil
}

type MongoDBMetadata struct {
	BackupName string          `json:"name,omitempty"`
	BackupID   string          `json:"id,omitempty"`
	BeforeTS   Timestamp       `json:"before_ts,omitempty"`
	AfterTS    Timestamp       `json:"after_ts,omitempty"`
	DataSize   int64           `json:"data_size,omitempty"`
	RootPath   string          `json:"root_path,omitempty"`
	RawMeta    json.RawMessage `json:"raw_meta,omitempty"`
	ShardNames []string        `json:"shard_names,omitempty"`
}

var _ metadb.BackupMetadata = MongoDBMetadata{}

func (meta MongoDBMetadata) ID() string {
	return meta.BackupID
}

func (meta MongoDBMetadata) Marshal() ([]byte, error) {
	return json.Marshal(meta)
}

func (meta MongoDBMetadata) String() string {
	raw, err := meta.Marshal()
	if err != nil {
		return fmt.Sprintf("can not marshal mongodb metadata with error %v: %+v", err, raw)
	}
	return string(raw)
}

func UnmarshalMongoDBMetadata(raw []byte) (MongoDBMetadata, error) {
	var meta MongoDBMetadata
	if err := json.Unmarshal(raw, &meta); err != nil {
		return MongoDBMetadata{}, err
	}
	return meta, nil
}

func (mbm *MongodbS3BackupManager) metadataFromS3Object(ctx context.Context, bucket string, obj s3.Object) (MongoDBMetadata, error) {
	r, err := mbm.s3.GetObject(ctx, bucket, obj.Key)
	if err != nil {
		return MongoDBMetadata{}, err
	}
	defer r.Close()

	rawMeta, err := ioutil.ReadAll(r)
	if err != nil {
		return MongoDBMetadata{}, err
	}
	root, name, err := walg.ParseWalgSentinelPath(obj.Key)
	if err != nil {
		return MongoDBMetadata{}, err
	}
	return MetadataFromMongoWalgSentinel(rawMeta, root, name)
}

func MetadataFromMongoWalgSentinel(raw []byte, root, name string) (MongoDBMetadata, error) {
	sentinel, err := UnmarshalMongoDBWalgSentinel(raw)
	if err != nil {
		return MongoDBMetadata{}, err
	}

	return MongoDBMetadata{
		BackupName: name,
		BackupID:   sentinel.UserData.BackupID,
		BeforeTS:   sentinel.MongoMeta.Before.LastMajTS,
		AfterTS:    sentinel.MongoMeta.After.LastMajTS,
		DataSize:   sentinel.DataSize,
		RootPath:   root,
		ShardNames: []string{sentinel.UserData.ShardName},
		RawMeta:    raw,
	}, nil
}

func UnmarshalMongoDBWalgSentinel(raw []byte) (MongoDBWalgSentinel, error) {
	var sentinel MongoDBWalgSentinel
	if err := json.Unmarshal(raw, &sentinel); err != nil {
		return MongoDBWalgSentinel{}, err
	}
	return sentinel, nil
}

type MongoDBWalgSentinel struct {
	BackupName      string    `json:"BackupName"`
	StartLocalTime  time.Time `json:"StartLocalTime"`
	FinishLocalTime time.Time `json:"FinishLocalTime"`
	UserData        UserData  `json:"UserData"`
	MongoMeta       MongoMeta `json:"MongoMeta"`
	Permanent       bool      `json:"Permanent"`
	DataSize        int64     `json:"DataSize"`
}

type UserData struct {
	BackupID  string `json:"backup_id"`
	ShardName string `json:"shard_name"`
}

// NodeMeta represents MongoDB node metadata
type NodeMeta struct {
	LastMajTS Timestamp `json:"LastMajTS"`
}

// MongoMeta includes NodeMeta Before and after backup
type MongoMeta struct {
	Before NodeMeta `json:"Before"`
	After  NodeMeta `json:"After"`
}

// Timestamp represents oplog record uniq id.
type Timestamp struct {
	TS  uint64 `json:"TS"`
	Inc uint32 `json:"Inc"`
}

type MongoDBJobFactsExtractor struct {
}

func NewMongoDBJobFactsExtractor() *MongoDBJobFactsExtractor {
	return &MongoDBJobFactsExtractor{}
}

func (m *MongoDBJobFactsExtractor) BackupPathFromJob(ctx context.Context, job models.BackupJob) (string, error) {
	replSetID := job.ShardID.String
	if !job.ShardID.Valid {
		replSetID = job.SubClusterID
	}
	return MongodbBackupPathFromReplset(job.ClusterID, replSetID), nil
}

func (m *MongoDBJobFactsExtractor) BackupNameRootFromJob(ctx context.Context, job models.BackupJob) (string, string, error) {
	meta, err := UnmarshalMongoDBMetadata(job.Metadata)
	if err != nil {
		return "", "", xerrors.Errorf("can not unmarshal mongodb metadata with error: %w", err)
	}
	return meta.BackupName, meta.RootPath, nil
}

type MongoDBCreationPillar struct {
	BackupID          string `json:"backup_id"`
	BackupIsPermanent bool   `json:"backup_is_permanent"`
}

type MongoDBDeletionPillar struct {
	BackupID   string `json:"backup_id"`
	BackupName string `json:"backup_name"`
	BackupRoot string `json:"backup_root"`
}

type MongoDBDeployArgsProvider struct {
}

func NewMongoDBDeployArgsProvider() *MongoDBDeployArgsProvider {
	return &MongoDBDeployArgsProvider{}
}

func (m *MongoDBDeployArgsProvider) CreationArgsFromJob(ctx context.Context, job models.BackupJob, deployType backupmanager.DeployType) ([]string, error) {
	if deployType != backupmanager.StateDeployType {
		return nil, xerrors.Errorf("unexpected deploy type")
	}

	pillar := MongoDBCreationPillar{
		BackupID:          job.BackupID,
		BackupIsPermanent: job.Initiator == metadb.BackupInitiatorUser,
	}

	return backupmanager.PillarToArgs(pillar)
}

func (m *MongoDBDeployArgsProvider) DeletionArgsFromJob(ctx context.Context, job models.BackupJob) ([]string, error) {
	meta, err := UnmarshalMongoDBMetadata(job.Metadata)
	if err != nil {
		return nil, err
	}

	pillar := MongoDBDeletionPillar{
		BackupID:   job.BackupID,
		BackupName: meta.BackupName,
		BackupRoot: meta.RootPath,
	}

	return backupmanager.PillarToArgs(pillar)
}
