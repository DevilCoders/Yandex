package provider

import (
	"context"
	"fmt"
	"time"

	"a.yandex-team.ru/cloud/mdb/backup/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/backup/worker/internal/jobproducer"
	"a.yandex-team.ru/cloud/mdb/backup/worker/internal/models"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
)

type JobProducer struct {
	name        string
	lg          log.Logger
	produceFunc jobproducer.ProduceBackupJobFunc
	mdb         metadb.MetaDB
}

func NewJobProducer(
	name string,
	mdb metadb.MetaDB,
	lg log.Logger,
	produceFunc jobproducer.ProduceBackupJobFunc) *JobProducer {
	return &JobProducer{
		name:        name,
		lg:          lg.WithName(fmt.Sprintf("%s_job_producer", name)),
		produceFunc: produceFunc,
		mdb:         mdb,
	}
}

func (jp *JobProducer) ProduceBackupJob(ctx context.Context, ch chan models.BackupJob) (err error) {
	var jobCtx context.Context
	jobCtx, err = jp.mdb.Begin(ctx, sqlutil.Primary)
	if err != nil {
		return
	}
	// This is a must, otherwise we can leak transaction
	defer func() {
		if r := recover(); r != nil {
			_ = jp.mdb.Rollback(jobCtx)
			panic(r)
		}
		if err != nil {
			_ = jp.mdb.Rollback(jobCtx)
		}
	}()

	var b metadb.Backup
	b, err = jp.produceFunc(jobCtx)
	if err != nil {
		return
	}
	jobCtx = ctxlog.WithFields(jobCtx, log.String("backup_id", b.BackupID))
	ctxlog.Debugf(jobCtx, jp.lg, "fetched backup: %+v", b)
	ts := time.Now()
	ch <- models.NewBackupJob(jobCtx, ts, b)
	ctxlog.Debug(jobCtx, jp.lg, "passed to handler queue", log.Duration("queue_wait", time.Since(ts)))
	return
}
