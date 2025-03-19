package provider

import (
	"context"
	"encoding/json"
	"sort"
	"time"

	"a.yandex-team.ru/cloud/mdb/backup/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/backup/scheduler/internal/models"
	"a.yandex-team.ru/cloud/mdb/internal/generator"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type IncrementalBackupArgsExtractor struct {
	mdb          metadb.MetaDB
	lg           log.Logger
	genExtractor *GenericBackupArgsExtractor
}

type IncrementalScheduleInfo struct {
	IncrementalSteps int `json:"max_incremental_steps"`
}

func NewIncrementalBackupArgsExtractor(mdb metadb.MetaDB, lg log.Logger) *IncrementalBackupArgsExtractor {
	return &IncrementalBackupArgsExtractor{
		mdb:          mdb,
		lg:           lg,
		genExtractor: NewGenericBackupArgsExtractor(),
	}
}

func (ibg *IncrementalBackupArgsExtractor) GetBackupArgs(ctx context.Context, blank metadb.BackupBlank, backupIDGen generator.IDGenerator) (metadb.CreateBackupArgs, error) {
	backups, err := ibg.mdb.ListBackups(ctx, blank.ClusterID, optional.String{}, optional.String{}, metadb.StatusesActive, []metadb.BackupInitiator{metadb.BackupInitiatorSchedule})
	if err != nil {
		return metadb.CreateBackupArgs{}, xerrors.Errorf("failed to get backup state info: %w", err)
	}

	var schedInfo IncrementalScheduleInfo
	scheduleRaw, err := ibg.mdb.BackupSchedule(ctx, blank.ClusterID)
	if err != nil {
		return metadb.CreateBackupArgs{}, xerrors.Errorf("failed to get backup state info: %w", err)
	}

	err = json.Unmarshal([]byte(scheduleRaw), &schedInfo)
	if err != nil {
		return metadb.CreateBackupArgs{}, xerrors.Errorf("failed to parse %v  schedule info: %w", blank.ClusterID, err)
	}

	backupArgs, err := ibg.genExtractor.GetBackupArgs(ctx, blank, backupIDGen)
	if err != nil {
		return metadb.CreateBackupArgs{}, xerrors.Errorf("cluster %s: failed to get generic backup args: %w", blank.ClusterID, err)
	}

	var backupDep string

	if len(backups) > 0 {

		sort.SliceStable(backups, func(i, j int) bool {
			return backups[i].FinishedAt.Time.After(backups[j].FinishedAt.Time)
		})

		targetBackup := backups[0]
		incInfo, err := models.ParseIncrementalInfo(targetBackup.Metadata, blank.ClusterType)
		if err != nil {
			return metadb.CreateBackupArgs{}, xerrors.Errorf("failed to parse %v metadata info: %w", blank.ClusterID, err)
		}

		ibg.lg.Debugf("meta last: %v sched info: %v %v", incInfo, schedInfo, string(scheduleRaw))

		if ibg.checkIfIncrementPossible(ctx, blank, incInfo, schedInfo, targetBackup) {
			backupDep = targetBackup.BackupID
			backupArgs.Method = metadb.BackupMethodIncremental
		}
	}

	id, err := backupIDGen.Generate()
	if err != nil {
		return metadb.CreateBackupArgs{}, xerrors.Errorf("backup id not generated: %w", err)
	}
	backupArgs.BackupID = id
	if len(backupDep) > 0 {
		backupArgs.DependsOnBackupIDs = []string{backupDep}
	}

	return backupArgs, nil
}

func (ibg *IncrementalBackupArgsExtractor) checkIfIncrementPossible(
	ctx context.Context, blank metadb.BackupBlank, incInfo models.IncrementalInfo, schedInfo IncrementalScheduleInfo, targetBackup metadb.Backup) bool {

	currMajor, err := ibg.fetchClusterMajorVersion(ctx, blank.ClusterType, blank.ClusterID, time.Now())
	if err != nil {
		ibg.lg.Warnf("failed to fetch %s cluster current major version: %s, choosing a full backup method instead of incremental", blank.ClusterID, err)
		return false
	}

	if !targetBackup.FinishedAt.Valid {
		ibg.lg.Warnf("failed to fetch %s cluster target backup %s finish ts, choosing a full backup method instead of incremental", blank.ClusterID, targetBackup.BackupID)
		return false
	}

	targetMajor, err := ibg.fetchClusterMajorVersion(ctx, targetBackup.ClusterType, targetBackup.ClusterID, targetBackup.FinishedAt.Time)
	if err != nil {
		ibg.lg.Warnf("failed to fetch %s target backup %s cluster major version: %s, choosing a full backup method instead of incremental", blank.ClusterID, targetBackup.BackupID, err)
		return false
	}

	if currMajor != targetMajor {
		ibg.lg.Warnf(
			"cluster %s: %s backup cluster major version %v mismatch the current cluster major version %v, choosing a full backup method instead of incremental",
			blank.ClusterID, targetBackup.BackupID, currMajor, targetMajor)
		return false
	}

	return !incInfo.IsIncremental || schedInfo.IncrementalSteps > incInfo.IncrementCount
}

func (ibg *IncrementalBackupArgsExtractor) fetchClusterMajorVersion(ctx context.Context, ctype metadb.ClusterType, clusterID string, ts time.Time) (string, error) {
	componentName, err := getComponentByClusterType(ctype)
	if err != nil {
		return "", xerrors.Errorf("Failed to get component name of cluster ")
	}
	versionByComponent, err := ibg.mdb.ClusterVersionsAtTS(ctx, clusterID, ts)
	if err != nil {
		return "", xerrors.Errorf("Failed to get cluster versions: %w", err)
	}
	componentVersions, ok := versionByComponent[componentName]
	if !ok {
		return "", xerrors.Errorf("There is no entry for cluster version in metadb")
	}
	return componentVersions.MajorVersion, nil
}

func getComponentByClusterType(ctype metadb.ClusterType) (string, error) {
	switch ctype {
	case metadb.PostgresqlCluster:
		return "postgres", nil
	case metadb.MysqlCluster:
		return "mysql", nil
	case metadb.MongodbCluster:
		return "mongodb", nil
	case metadb.ClickhouseCluster:
		return "clickhouse", nil
	case metadb.RedisCluster:
		return "redis", nil
	case metadb.ElasticSearchCluster:
		return "elasticsearch", nil
	case metadb.SQLServerCluster:
		return "sqlserver", nil
	case metadb.HadoopCluster:
		return "hadoop", nil
	case metadb.KafkaCluster:
		return "kafka", nil
	default:
		return "", xerrors.New("not supported cluster type")
	}
}
