package provider

import (
	"fmt"
	"time"

	"a.yandex-team.ru/cloud/mdb/backup/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/backup/worker/internal/delayer"
	"a.yandex-team.ru/cloud/mdb/backup/worker/internal/executer"
	"a.yandex-team.ru/cloud/mdb/backup/worker/internal/models"
	"a.yandex-team.ru/cloud/mdb/internal/encodingutil"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/retry"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type Config struct {
	MaxFinalizeRetries int                   `json:"max_finalize_retries" yaml:"max_finalize_retries"`
	DefaultTimeout     encodingutil.Duration `json:"default_timeout" yaml:"default_timeout"`
}

func DefaultConfig() Config {
	return Config{
		MaxFinalizeRetries: 3,
		DefaultTimeout:     encodingutil.FromDuration(24 * time.Hour),
	}
}

type GenericJobHandler struct {
	name          string
	lg            log.Logger
	cfg           Config
	finishRetry   *retry.BackOff
	nextDelayFunc delayer.NextDelayFunc
	execFuncs     map[metadb.ClusterType]executer.ExecFunc
	mdb           metadb.MetaDB
}

func NewGenericJobHandler(
	name string,
	mdb metadb.MetaDB,
	lg log.Logger,
	execFuncs map[metadb.ClusterType]executer.ExecFunc,
	nextDelayFunc delayer.NextDelayFunc,
	cfg Config) *GenericJobHandler {
	return &GenericJobHandler{
		name:          name,
		lg:            lg.WithName(fmt.Sprintf("%s_job_handler", name)),
		cfg:           cfg,
		finishRetry:   retry.New(retry.Config{MaxRetries: uint64(cfg.MaxFinalizeRetries)}),
		nextDelayFunc: nextDelayFunc,
		execFuncs:     execFuncs,
		mdb:           mdb,
	}
}

func (jh *GenericJobHandler) HandleBackupJob(job models.BackupJob) error {
	defer ctxlog.Debug(job.Ctx, jh.lg, "job completed", log.Duration("duration", time.Since(job.FetchedTS)))
	defer jh.mdb.Rollback(job.Ctx)

	exc, ok := jh.execFuncs[job.ClusterType]
	if !ok {
		return metadb.UnsupportedClusterTypeError
	}
	results := exc(job.Ctx, job)
	if len(results) < 1 {
		return xerrors.Errorf("empty executer results for backup %s", job.BackupID)
	}

	delay, timedOut := jh.nextDelayFunc(job.DelayedUntil, jh.cfg.DefaultTimeout.Duration)
	lg := jh.lg.WithName(job.BackupID)

	var finalizers []func() error
	for _, r := range results {
		switch res := r.(type) {
		case executer.BackupCreationStart:
			// temporary fix https://st.yandex-team.ru/MDB-15749
			// here should be requested start time instead of createdAt
			// createdAt works until we decide to plan backups to future
			delay, timedOut := jh.nextDelayFunc(job.CreatedAt, jh.cfg.DefaultTimeout.Duration)

			// Looks like we can generalize following functions, but won't do that
			// it is highly likely that the parameters will change
			finalizers = append(finalizers, func() error { return finalizeBackupCreationStart(job, res, jh.mdb, lg, delay, timedOut) })
		case executer.BackupCreationComplete:
			finalizers = append(finalizers, func() error { return finalizeBackupCompletion(job, res, jh.mdb, lg, delay, timedOut) })
		case executer.BackupDeletionStart:
			finalizers = append(finalizers, func() error { return finalizeBackupDeletionStart(job, res, jh.mdb, lg, delay, timedOut) })
		case executer.BackupDeletionComplete:
			finalizers = append(finalizers, func() error { return finalizeBackupDeletion(job, res, jh.mdb, lg, delay, timedOut) })
		case executer.BackupResize:
			finalizers = append(finalizers, func() error {
				return jh.mdb.SetBackupSize(job.Ctx, res.ID, res.DataSize, res.JournalSize, time.Now())
			})
		default:
			return xerrors.Errorf("unsupported executer result: %+v", r)
		}
	}

	return jh.finishRetry.RetryWithLog(
		job.Ctx,
		func() error {
			for _, f := range finalizers {
				if err := f(); err != nil {
					return err
				}
			}
			return jh.mdb.Commit(job.Ctx)
		},
		fmt.Sprintf("retrying finalize %q", job.BackupID),
		lg,
	)
}

func finalizeBackupCreationStart(job models.BackupJob, res executer.BackupCreationStart, mdb metadb.MetaDB, lg log.Logger, delay time.Duration, timedOut bool) error {
	ctx := job.Ctx
	backupID := job.BackupID
	if res.Err == nil {
		return mdb.CompleteBackupCreationStart(ctx, backupID, res.ShipmentID)
	}

	merr := metadb.FromError(res.Err)
	if merr.Temporary() {
		ctxlog.Warn(ctx, lg, "temporary error appeared during backup creation starting", log.Error(res.Err))
		if !timedOut {
			delayUntil := time.Now().Add(delay)
			ctxlog.Warnf(ctx, lg, "backup job delayed for %s until %s", delay, delayUntil.Format(time.RFC3339))
			return mdb.DelayPendingBackupUntil(ctx, backupID, delayUntil, job.Errors)
		}
		job.Errors.Add(*merr)
		merr = metadb.TimedOutError
	}
	ctxlog.Error(ctx, lg, "permanent error appeared during backup creation starting", log.Error(merr))
	job.Errors.Add(*merr)

	return mdb.FailBackupCreation(ctx, backupID, job.Errors)
}

func finalizeBackupCompletion(job models.BackupJob, res executer.BackupCreationComplete, mdb metadb.MetaDB, lg log.Logger, delay time.Duration, timedOut bool) error {
	ctx := job.Ctx
	backupID := job.BackupID

	if res.Err == nil {
		return mdb.CompleteBackupCreation(ctx, backupID, optional.Time{}, res.Metadata)
	}

	merr := metadb.FromError(res.Err)
	if merr.Temporary() {
		ctxlog.Warn(ctx, lg, "temporary error appeared during backup completion", log.Error(res.Err))
		if !timedOut {
			delayUntil := time.Now().Add(delay)
			ctxlog.Warnf(ctx, lg, "backup job delayed for %s until %s", delay, delayUntil.Format(time.RFC3339))
			return mdb.DelayPendingBackupUntil(ctx, backupID, delayUntil, job.Errors)
		}
		job.Errors.Add(*merr)
		merr = metadb.TimedOutError
	}
	ctxlog.Error(ctx, lg, "permanent error appeared during backup creation", log.Error(merr))
	job.Errors.Add(*merr)

	return mdb.FailBackupCreation(ctx, backupID, job.Errors)
}

func finalizeBackupDeletionStart(job models.BackupJob, res executer.BackupDeletionStart, mdb metadb.MetaDB, lg log.Logger, delay time.Duration, timedOut bool) error {
	ctx := job.Ctx
	backupID := job.BackupID
	if res.Err == nil {
		return mdb.CompleteBackupDeletionStart(ctx, backupID, res.ShipmentID)
	}

	merr := metadb.FromError(res.Err)
	if merr.Temporary() {
		ctxlog.Warn(ctx, lg, "temporary error appeared during backup deletion starting", log.Error(res.Err))
		if !timedOut {
			delayUntil := time.Now().Add(delay)
			lg.Warnf("backup job delayed for %s until %s", delay, delayUntil.Format(time.RFC3339))
			return mdb.DelayPendingBackupUntil(ctx, backupID, delayUntil, job.Errors)
		}
		job.Errors.Add(*merr)
		merr = metadb.TimedOutError
	}
	lg.Error("permanent error appeared during backup deletion starting", log.Error(merr))
	job.Errors.Add(*merr)

	return mdb.FailBackupDeletion(ctx, backupID, job.Errors)
}

func finalizeBackupDeletion(job models.BackupJob, res executer.BackupDeletionComplete, mdb metadb.MetaDB, lg log.Logger, delay time.Duration, timedOut bool) error {
	ctx := job.Ctx
	backupID := job.BackupID

	if res.Err == nil {
		return mdb.CompleteBackupDeletion(ctx, backupID)
	}

	merr := metadb.FromError(res.Err)
	if merr.Temporary() {
		lg.Warn("temporary error appeared during backup deletion", log.Error(merr))
		if !timedOut {
			delayUntil := time.Now().Add(delay)
			ctxlog.Warnf(ctx, lg, "backup job delayed for %s until %s", delay, delayUntil.Format(time.RFC3339))
			return mdb.DelayPendingBackupUntil(ctx, backupID, delayUntil, job.Errors)
		}
		job.Errors.Add(*merr)
		merr = metadb.TimedOutError
	}
	lg.Error("permanent error appeared during backup deletion", log.Error(merr))
	job.Errors.Add(*merr)

	return mdb.FailBackupDeletion(ctx, backupID, job.Errors)
}
