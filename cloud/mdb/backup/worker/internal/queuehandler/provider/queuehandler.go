package provider

import (
	"fmt"
	"sync"

	"a.yandex-team.ru/cloud/mdb/backup/worker/internal/jobhandler"
	"a.yandex-team.ru/cloud/mdb/backup/worker/internal/models"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
)

type Config struct {
	Parallel int `json:"parallel" yaml:"parallel"`
}

func DefaultConfig() Config {
	return Config{
		Parallel: 3,
	}
}

type QueueHandler struct {
	name       string
	lg         log.Logger
	cfg        Config
	jobHandler jobhandler.JobHandler
}

func NewQueueHandler(name string, jobHandler jobhandler.JobHandler, cfg Config, lg log.Logger) *QueueHandler {
	return &QueueHandler{
		name:       name,
		lg:         lg.WithName(fmt.Sprintf("%s_queue_handler", name)),
		cfg:        cfg,
		jobHandler: jobHandler,
	}
}

func (qh *QueueHandler) HandleQueue(jobCh chan models.BackupJob) chan error {
	wg := &sync.WaitGroup{}
	errch := make(chan error)

	for i := 0; i < qh.cfg.Parallel; i++ {
		wg.Add(1)
		go func(num int) {
			lg := qh.lg.WithName(fmt.Sprintf("%d", num))
			lg.Debug("started")
			defer func() {
				wg.Done()
				lg.Debug("finished")
			}()

			for job := range jobCh {
				if err := qh.jobHandler.HandleBackupJob(job); err != nil {
					ctxlog.Error(job.Ctx, qh.lg, "can not handle job, going to request termination", log.Error(err))
					errch <- fmt.Errorf("failed job %q with error: %+v", job.BackupID, err)
				}
			}
		}(i)
	}

	go func() {
		wg.Wait()
		close(errch)
		qh.lg.Info("queue exhausted, handlers have exited")
	}()

	return errch
}
