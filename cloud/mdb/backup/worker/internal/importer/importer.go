package importer

import (
	"context"
)

type Importer interface {
	Import(ctx context.Context, cid string, dryrun bool) (ImportStats, error)
}

type ImportStats struct {
	ExistsInStorage           int
	ExistsInMetadb            int
	ImportedIntoMetadb        int
	CompletedCreation         int
	SkippedDueToExistence     int
	SkippedDueToDryRun        int
	SkippedDueToUniqSchedDate int
	SkippedNoIncrementBase    int
}

func (is *ImportStats) Append(other ImportStats) {
	is.ExistsInStorage += other.ExistsInStorage
	is.ExistsInMetadb += other.ExistsInMetadb
	is.ImportedIntoMetadb += other.ImportedIntoMetadb
	is.CompletedCreation += other.CompletedCreation
	is.SkippedDueToExistence += other.SkippedDueToExistence
	is.SkippedDueToDryRun += other.SkippedDueToDryRun
	is.SkippedDueToUniqSchedDate += other.SkippedDueToUniqSchedDate
	is.SkippedNoIncrementBase += other.SkippedNoIncrementBase
}
