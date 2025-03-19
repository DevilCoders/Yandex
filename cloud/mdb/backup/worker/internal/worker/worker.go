package worker

import (
	"context"
	"sync"
	"time"

	"a.yandex-team.ru/cloud/mdb/backup/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/backup/worker/internal/queuehandler"
	"a.yandex-team.ru/cloud/mdb/backup/worker/internal/queueproducer"
	"a.yandex-team.ru/cloud/mdb/internal/encodingutil"
	"a.yandex-team.ru/library/go/core/log"
)

type Pipeline struct {
	Name     string
	Producer queueproducer.QueueProducer
	Handler  queuehandler.QueueHandler
}

func NewPipeline(name string, qproducer queueproducer.QueueProducer, qhandler queuehandler.QueueHandler) Pipeline {
	return Pipeline{name, qproducer, qhandler}
}

type Config struct {
	GracefulShutdownTimeout encodingutil.Duration `json:"graceful_shutdown_timeout" yaml:"graceful_shutdown_timeout"`
}

func DefaultConfig() Config {
	return Config{GracefulShutdownTimeout: encodingutil.FromDuration(30 * time.Second)}
}

type Worker struct {
	mdb       metadb.MetaDB
	lg        log.Logger
	pipelines []Pipeline
	cfg       Config
}

func NewWorker(
	mdb metadb.MetaDB,
	pipelines []Pipeline,
	cfg Config,
	lg log.Logger,
) *Worker {
	return &Worker{mdb, lg, pipelines, cfg}
}

func (w *Worker) IsReady(ctx context.Context) error {
	return w.mdb.IsReady(ctx)
}

func (w *Worker) Run(ctx context.Context) error {
	var errChans []<-chan error

	// We have two contexts here.
	// produceCtx cancels producers cycles.
	// It inherits from incoming context and used to start graceful shutdown.
	produceCtx, produceCancel := context.WithCancel(ctx)
	defer produceCancel()
	// jobCtx cancels in-progress job handlers.
	// It's _independent_ context and cancelled after graceful shutdown timeout.
	jobsCtx, jobsCancel := context.WithCancel(context.Background())
	defer jobsCancel()

	for _, p := range w.pipelines {
		jobsChan, producerErrChan := p.Producer.ProduceQueue(produceCtx, jobsCtx)
		errChans = append(errChans, producerErrChan)

		handlerErrChan := p.Handler.HandleQueue(jobsChan)
		errChans = append(errChans, handlerErrChan)

		w.lg.Infof("%s pipeline started", p.Name)
	}

	errch := MergeErrors(errChans...)
	var firsterr error

	// Wait for first error from all parts of pipeline
	// or
	// parent context cancel (usually stop on sigterm).
	// Then stop producerCtx for graceful shutdown:
	// current jobs keep running,
	// handlers should process them all and close their error channels.

	select {
	case firsterr = <-errch:
		w.lg.Error("error fetched, shutting down", log.Error(firsterr))
	case <-ctx.Done():
		w.lg.Info("termination requested, shutting down")
	}
	produceCancel()

	// timer for graceful shutdown started
	jobsStopTimer := time.NewTimer(w.cfg.GracefulShutdownTimeout.Duration)
	defer func() {
		if !jobsStopTimer.Stop() {
			select {
			case <-jobsStopTimer.C:
			default:
			}
		}
	}()

	// If the common error channel is closed, it means that all pipelines have exited.
	// The first found error will be returned, the rest are logged.
	// If timer expires, jobsCtx is cancelled to stop all running jobs.
	for {
		select {
		case err := <-errch:
			if err == nil {
				w.lg.Info("all cycles have finished")
				return firsterr
			}
			w.lg.Error("error after producer stop", log.Error(err))
			if firsterr == nil {
				firsterr = err
			}
		case <-jobsStopTimer.C:
			w.lg.Warnf("graceful shutdown timeout (%s) passed, stopping jobs context", w.cfg.GracefulShutdownTimeout.Duration)
			jobsCancel()
		}
	}
}

func MergeErrors(cs ...<-chan error) <-chan error {
	var wg sync.WaitGroup
	out := make(chan error, len(cs))
	output := func(c <-chan error) {
		for n := range c {
			out <- n
		}
		wg.Done()
	}
	wg.Add(len(cs))
	for _, c := range cs {
		go output(c)
	}

	go func() {
		wg.Wait()
		close(out)
	}()
	return out
}
