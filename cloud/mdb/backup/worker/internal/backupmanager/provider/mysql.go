package provider

import (
	"context"
	"encoding/json"
	"fmt"
	"io/ioutil"
	"strconv"
	"strings"

	"a.yandex-team.ru/cloud/mdb/backup/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/backup/internal/tools/walg"
	"a.yandex-team.ru/cloud/mdb/backup/internal/tools/walg/mysql"
	"a.yandex-team.ru/cloud/mdb/backup/worker/internal/backupmanager"
	"a.yandex-team.ru/cloud/mdb/backup/worker/internal/models"
	"a.yandex-team.ru/cloud/mdb/internal/s3"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/ptr"
)

type MySQLS3BackupManager struct {
	s3 s3.Client
	lg log.Logger
}

func NewMySQLBackupManager(s3 s3.Client, lg log.Logger) *MySQLS3BackupManager {
	return &MySQLS3BackupManager{s3: s3, lg: lg}
}

func mySQLWalgRootPath(cid string, majorVersion int) string {
	return fmt.Sprintf("wal-g/%s/%d/", cid, majorVersion)
}

func (mybm *MySQLS3BackupManager) ListBackups(ctx context.Context, bucket, prefixPath string) ([]metadb.BackupMetadata, error) {
	objects, _, err := mybm.s3.ListObjects(ctx, bucket, s3.ListObjectsOpts{Prefix: ptr.String(prefixPath)})
	if err != nil {
		return nil, err
	}
	backups := make([]metadb.BackupMetadata, 0, len(objects))

	for _, obj := range objects {
		// if backup does not have a sentinel file, it is considered invalid,
		// we should ignore such backups
		if !strings.HasSuffix(obj.Key, walg.WalgSentinelSuffix) {
			mybm.lg.Debug("skip object", log.String("key", obj.Key))
			continue
		}

		backup, err := mybm.metadataFromS3Object(ctx, bucket, obj)
		if err != nil {
			return nil, err
		}

		backups = append(backups, backup)
	}
	return backups, nil
}
func (mybm *MySQLS3BackupManager) metadataFromS3Object(ctx context.Context, bucket string, obj s3.Object) (mysql.MySQLMetadata, error) {
	// Sentinel: ****_backup_stop_sentinel.json - generic info about the backup

	sentinel, rSentinel, err := fetchStreamSentinelDtoFromS3(ctx, mybm.s3, bucket, obj.Key)
	if err != nil {
		return mysql.MySQLMetadata{}, xerrors.Errorf("fetch backup_stop_sentinel.json from S3: %w", err)
	}

	rootPath, backupName, err := walg.ParseWalgSentinelPath(obj.Key)
	if err != nil {
		return mysql.MySQLMetadata{}, err
	}

	return mysql.NewMySQLMetadata(backupName, rootPath, sentinel, rSentinel, sentinel)
}

func fetchStreamSentinelDtoFromS3(ctx context.Context, s3 s3.Client, bucket, sentinelPath string) (mysql.StreamSentinelDto, json.RawMessage, error) {
	rSentinel, err := s3.GetObject(ctx, bucket, sentinelPath)
	if err != nil {
		return mysql.StreamSentinelDto{}, nil, err
	}
	defer rSentinel.Close()

	rawSentinel, err := ioutil.ReadAll(rSentinel)
	if err != nil {
		return mysql.StreamSentinelDto{}, []byte{}, err
	}

	sentinel, err := mysql.UnmarshalStreamSentinelDto(rawSentinel)
	if err != nil {
		return mysql.StreamSentinelDto{}, []byte{}, err
	}

	return sentinel, rawSentinel, nil
}

type MySQLJobFactsExtractor struct {
	mdb metadb.MetaDB
}

func NewMySQLJobFactsExtractor(mdb metadb.MetaDB) *MySQLJobFactsExtractor {
	return &MySQLJobFactsExtractor{
		mdb: mdb,
	}
}

func mysqlMajorVersion2Number(version string) (int, error) {
	s := strings.Split(version, ".")
	if len(s) != 2 {
		return 0, xerrors.Errorf("incorrect major mysql version format: %s", version)
	}
	num := 0
	for i := 0; i <= 1; i++ {
		current, err := strconv.Atoi(s[i])
		if err != nil {
			return 0, err
		}
		num *= 100
		num += current
	}
	return num, nil
}

func ParseMysqlMajorVersion(ctx context.Context, mdb metadb.MetaDB, clusterID string) (int, error) {
	versionByComponent, err := mdb.ClusterVersions(ctx, clusterID)
	if err != nil {
		return 0, xerrors.Errorf("get cluster versions: %w", err)
	}
	mysqlVersions, ok := versionByComponent["mysql"]
	if !ok {
		return 0, xerrors.Errorf("no entry for mysql component version in cluster versions")
	}
	majorVersionNum, err := mysqlMajorVersion2Number(mysqlVersions.MajorVersion)
	if err != nil {
		return 0, xerrors.Errorf("parse mysql major version number: %w", err)
	}
	return majorVersionNum, nil
}

func MySQLBackupPathFromCidAndVersion(clusterID string, majorVersion int) string {
	return walg.WalgBackupsPathFromRootPath(mySQLWalgRootPath(clusterID, majorVersion))
}

func (myde *MySQLJobFactsExtractor) BackupPathFromJob(ctx context.Context, job models.BackupJob) (string, error) {
	meta, err := mysql.UnmarshalMySQLMetadata(job.Metadata)
	if err == nil {
		return walg.WalgBackupsPathFromRootPath(meta.RootPath), nil
	}

	myVersion, err := ParseMysqlMajorVersion(ctx, myde.mdb, job.ClusterID)
	if err != nil {
		return "", metadb.WrapTempError(err, "can not get cluster Postgres major version")
	}
	return MySQLBackupPathFromCidAndVersion(job.ClusterID, myVersion), nil
}

func (myde *MySQLJobFactsExtractor) BackupNameRootFromJob(ctx context.Context, job models.BackupJob) (string, string, error) {
	meta, err := mysql.UnmarshalMySQLMetadata(job.Metadata)
	if err != nil {
		return "", "", xerrors.Errorf("can not unmarshal mysql metadata with error: %w", err)
	}
	return meta.BackupName, meta.RootPath, nil
}

type MySQLDeployArgsProvider struct {
	lg log.Logger
}

func NewMySQLDeployArgsProvider() *MySQLDeployArgsProvider {
	return &MySQLDeployArgsProvider{}
}

type MySQLCreationPillar struct {
	BackupID          string `json:"backup_id"`
	BackupIsPermanent bool   `json:"user_backup,omitempty"`
}

type MySQLDeletionPillar struct {
	BackupID   string `json:"backup_id"`
	BackupName string `json:"backup_name"`
}

func (mydp *MySQLDeployArgsProvider) CreationArgsFromJob(ctx context.Context, job models.BackupJob, deployType backupmanager.DeployType) ([]string, error) {
	if deployType != backupmanager.StateDeployType {
		return nil, xerrors.Errorf("unexpected deploy type")
	}

	pillar := MySQLCreationPillar{
		BackupID:          job.BackupID,
		BackupIsPermanent: job.Initiator == metadb.BackupInitiatorUser,
	}

	return backupmanager.PillarToArgs(pillar)
}

func (mydp *MySQLDeployArgsProvider) DeletionArgsFromJob(ctx context.Context, job models.BackupJob) ([]string, error) {
	meta, err := mysql.UnmarshalMySQLMetadata(job.Metadata)
	if err != nil {
		return nil, err
	}

	pillar := MySQLDeletionPillar{
		BackupID:   job.BackupID,
		BackupName: meta.BackupName,
	}
	return backupmanager.PillarToArgs(pillar)
}
