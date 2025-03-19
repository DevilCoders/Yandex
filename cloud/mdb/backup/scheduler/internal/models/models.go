package models

import (
	"a.yandex-team.ru/cloud/mdb/backup/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/backup/internal/tools/walg/postgresql"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type IncrementalInfo struct {
	BackupID       string
	IsIncremental  bool
	IncrementCount int
}

func ParsePostgreSQLIncrementalInfo(metadata []byte) (IncrementalInfo, error) {
	meta, err := postgresql.UnmarshalPostgreSQLMetadata(metadata)
	if err != nil {
		return IncrementalInfo{}, err
	}

	incInfo := IncrementalInfo{
		BackupID:      meta.BackupID,
		IsIncremental: meta.IsIncremental,
	}
	if incInfo.IsIncremental {
		incInfo.IncrementCount = meta.IncrementDetails.IncrementCount
	}
	return incInfo, nil
}

func ParseIncrementalInfo(metadata []byte, ctype metadb.ClusterType) (IncrementalInfo, error) {
	switch ctype {
	case metadb.PostgresqlCluster:
		return ParsePostgreSQLIncrementalInfo(metadata)
	default:
		return IncrementalInfo{}, xerrors.New("not supported cluster type")
	}
}

type PurgeStats struct {
	Purged             int64
	SkippedDueToDryRun int64
}

type PlannedStats struct {
	Planned            int64
	SkippedDueToDryRun int64
}

type ObsoleteStats struct {
	Obsolete           int64
	SkippedDueToDryRun int64
}
