package models

import (
	"context"
	"time"

	"a.yandex-team.ru/cloud/mdb/backup/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/deploy/api/pkg/models"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type PlannedResult struct {
	BackupID   string
	ShipmentID string
	Err        error
	Errors     metadb.Errors
	Ctx        context.Context
}

type CreatingResult struct {
	BackupID string
	Metadata metadb.BackupMetadata
	Err      error
	Errors   metadb.Errors
	Ctx      context.Context
}

type ObsoleteResult struct {
	BackupID   string
	ShipmentID string
	Err        error
	Errors     metadb.Errors
	Ctx        context.Context
}

type DeletingResult struct {
	BackupID string
	Err      error
	Errors   metadb.Errors
	Ctx      context.Context
}

type BackupJob struct {
	metadb.Backup
	FetchedTS time.Time
	Ctx       context.Context
}

func NewBackupJob(ctx context.Context, fetchedTS time.Time, backup metadb.Backup) BackupJob {
	return BackupJob{Ctx: ctx, FetchedTS: fetchedTS, Backup: backup}
}

func PlannedResultFromJob(job BackupJob) PlannedResult {
	return PlannedResult{
		BackupID: job.BackupID,
		Ctx:      job.Ctx,
		Errors:   job.Errors,
	}
}

func CreatingResultFromJob(job BackupJob) CreatingResult {
	return CreatingResult{
		BackupID: job.BackupID,
		Ctx:      job.Ctx,
		Errors:   job.Errors,
	}
}

func ObsoleteResultFromJob(job BackupJob) ObsoleteResult {
	return ObsoleteResult{
		BackupID: job.BackupID,
		Ctx:      job.Ctx,
		Errors:   job.Errors,
	}
}

func DeletingResultFromJob(job BackupJob) DeletingResult {
	return DeletingResult{
		BackupID: job.BackupID,
		Ctx:      job.Ctx,
		Errors:   job.Errors,
	}
}

func ShipmentIDFromJob(job BackupJob) (models.ShipmentID, error) {
	jobShipmentID, err := job.ShipmentID.Get()
	if err != nil {
		return 0, xerrors.Errorf("empty value")
	}
	shipmentID, err := models.ParseShipmentID(jobShipmentID)
	if err != nil {
		return 0, xerrors.Errorf("invalid shipment id type: %s", err)
	}

	return shipmentID, nil
}
