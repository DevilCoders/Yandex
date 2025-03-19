package provider

import (
	"context"
	"fmt"
	"time"

	"a.yandex-team.ru/cloud/mdb/backup/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/backup/worker/internal/jobproducer"
	"a.yandex-team.ru/cloud/mdb/backup/worker/internal/models"
	"a.yandex-team.ru/cloud/mdb/internal/encodingutil"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type Config struct {
	OutputBufLen    int                   `json:"output_buf_len" yaml:"output_buf_len"`
	PendingInterval encodingutil.Duration `json:"pending_interval" yaml:"pending_interval"`
	MaxTasks        int32                 `json:"max_tasks" yaml:"max_tasks"`
}

func DefaultConfig() Config {
	return Config{
		OutputBufLen:    0,
		PendingInterval: encodingutil.FromDuration(time.Second * 10),
	}
}

type QueueProducer struct {
	name        string
	jobProducer jobproducer.JobProducer
	lg          log.Logger
	cfg         Config
}

func NewQueueProducer(name string, jobProducer jobproducer.JobProducer, cfg Config, lg log.Logger) *QueueProducer {
	return &QueueProducer{
		name:        name,
		jobProducer: jobProducer,
		cfg:         cfg,
		lg:          lg.WithName(fmt.Sprintf("%s_queue_producer", name)),
	}
}

func (qp *QueueProducer) ProduceQueue(produceCtx, jobCtx context.Context) (chan models.BackupJob, chan error) {
	var selected int32

	jobsch := make(chan models.BackupJob, qp.cfg.OutputBufLen)
	errch := make(chan error)

	go func() {
		qp.lg.Debug("started")
		timer := time.NewTimer(0)
		defer func() {
			if !timer.Stop() {
				select {
				case <-timer.C:
				default:
				}
			}
			close(jobsch)
			close(errch)
			qp.lg.Debug("finished")
		}()
		for {
			select {
			case <-produceCtx.Done():
				qp.lg.Info("context is done")
				return
			case <-timer.C:
				qp.lg.Info("wakeup by timer")
				for {
					err := qp.jobProducer.ProduceBackupJob(jobCtx, jobsch)
					if err != nil {
						if xerrors.Is(err, metadb.ErrDataNotFound) {
							break
						}
						err := fmt.Errorf("can not fetch backup job: %+v", err)
						qp.lg.Error("producer error", log.Error(err))
						errch <- err
						return
					}

					selected++
					if qp.cfg.MaxTasks > 0 && selected >= qp.cfg.MaxTasks {
						qp.lg.Infof("max_tasks to select has reached: %d", qp.cfg.MaxTasks)
						return
					}
				}

				timer.Reset(qp.cfg.PendingInterval.Duration)
			}
		}
	}()

	return jobsch, errch
}
