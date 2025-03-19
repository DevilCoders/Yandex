package internal

import (
	"context"
	"fmt"
	"time"

	"github.com/shirou/gopsutil/v3/mem"

	dm "a.yandex-team.ru/cloud/mdb/dataproc-agent/pkg/apiclient"
	jobconfig "a.yandex-team.ru/cloud/mdb/dataproc-agent/pkg/jobs/config"
	"a.yandex-team.ru/cloud/mdb/dataproc-agent/pkg/models"
	"a.yandex-team.ru/library/go/core/log"
)

type JobManagerAPI interface {
	Run(context.Context)
	Cancel(context.Context)
}

type JobManagerFactory func(*Orchestrator, models.Job) (JobManagerAPI, error)

type Orchestrator struct {
	config          jobconfig.Config
	dataprocManager dm.JobsAPI
	newJobManager   JobManagerFactory
	logger          log.Logger

	jobManagers             map[string]JobManagerAPI
	updatedJobs             map[string]models.Job
	jobManagerExitedChannel chan string
	jobUpdatedChannel       chan models.Job
}

func NewOrchestrator(config jobconfig.Config, api dm.JobsAPI, jobManagerFactory JobManagerFactory, logger log.Logger) *Orchestrator {
	return &Orchestrator{
		config:                  config,
		dataprocManager:         api,
		newJobManager:           jobManagerFactory,
		logger:                  logger,
		jobManagers:             make(map[string]JobManagerAPI),
		updatedJobs:             make(map[string]models.Job),
		jobManagerExitedChannel: make(chan string, 10),
		jobUpdatedChannel:       make(chan models.Job, 10),
	}
}

func (o *Orchestrator) isEnoughResources() bool {
	// Too many concurrent jobs produce many java processes that can cause out-of-memory crash
	vmStat, err := mem.VirtualMemory()
	if err != nil {
		o.logger.Error("Can not get memory stat.")
	} else if o.config.MaxConcurrentJobs > 1 && vmStat.Available < o.config.MinFreeMemoryToEnqueueNewJob {
		// checking o.config.MaxConcurrentJobs to avoid situation with small masternode
		// where at least one job should be runnable even if there is not enough memory
		o.logger.Infof(
			"Not enough resources to enqueue another job. Free RAM bytes: %d. Minimal free RAM bytes %d.",
			vmStat.Free,
			o.config.MinFreeMemoryToEnqueueNewJob,
		)
		return false
	}
	// Can not check only free memory at the moment of enqueuing
	// because tasks start to consume RAM later after being run
	// so we need to prevent out-of-memory ahead of time with a semaphore
	if len(o.jobManagers) >= o.config.MaxConcurrentJobs {
		o.logger.Infof(
			"Not enough queue slots to enqueue another job. Max queue size: %d",
			o.config.MaxConcurrentJobs,
		)
		return false
	}
	return true
}

func (o *Orchestrator) Run(ctx context.Context) {
	getNewJobsTicker := time.NewTicker(o.config.DataprocManager.GetNewJobsInterval)
	defer getNewJobsTicker.Stop()

	retryUpdateTicker := time.NewTicker(o.config.DataprocManager.RetryFailedRequestInterval)
	defer retryUpdateTicker.Stop()

	o.getNewJobs(ctx)
	for {
		select {
		case <-getNewJobsTicker.C:
			o.getNewJobs(ctx)
		case <-retryUpdateTicker.C:
			o.sendJobUpdates(ctx)
		case jobID := <-o.jobManagerExitedChannel:
			delete(o.jobManagers, jobID)
		case job := <-o.jobUpdatedChannel:
			o.updatedJobs[job.ID] = job
			o.enqueueAllJobUpdates()
			o.sendJobUpdates(ctx)
		case <-ctx.Done():
			return
		}
	}
}

func (o *Orchestrator) enqueueAllJobUpdates() {
	for {
		select {
		case job := <-o.jobUpdatedChannel:
			o.updatedJobs[job.ID] = job
		default:
			return
		}
	}
}

func (o *Orchestrator) getNewJobs(ctx context.Context) {
	jobStream := o.dataprocManager.ListActiveJobs()
	for {
		childCtx, cancel := context.WithTimeout(ctx, o.config.DataprocManager.RequestTimeout)
		job, err := jobStream.Next(childCtx)
		cancel()
		if err != nil {
			o.logger.Errorf("Failed to get new jobs from dataproc manager: %s", err)
			break
		}

		if job == nil {
			break
		}

		if job.State == models.JobStateCancelling {
			o.logger.Info("got job in JobStateCancelling")
			if o.jobManagerExists(*job) {
				o.jobManagers[job.ID].Cancel(ctx)
			} else if o.jobPendingInUpdatedChannel(*job) {
				continue
			} else {
				newJobInfo := *job
				newJobInfo.State = models.JobStateCancelled
				o.JobUpdated(newJobInfo)
			}
			continue
		}

		if !o.isEnoughResources() {
			continue
		}

		if o.jobManagerExists(*job) || o.jobPendingInUpdatedChannel(*job) {
			continue
		}

		jobManager, err := o.newJobManager(o, *job)
		if err != nil {
			newJobInfo := *job
			newJobInfo.State = models.JobStateError
			o.JobUpdated(newJobInfo)
			continue
		}

		o.jobManagers[job.ID] = jobManager
		go func() {
			jobManager.Run(ctx)
			o.jobManagerExitedChannel <- job.ID
		}()
	}
}

func (o *Orchestrator) jobManagerExists(job models.Job) bool {
	_, found := o.jobManagers[job.ID]
	return found
}

func (o *Orchestrator) jobPendingInUpdatedChannel(job models.Job) bool {
	// If we already ran the job in the past it is possible that JobManager is already unregistered,
	// but final job update (to DONE state) is still pending within jobUpdatedChannel.
	// So it is important to read all updates from jobUpdatedChannel here.
	o.enqueueAllJobUpdates()
	_, found := o.updatedJobs[job.ID]
	return found
}

func (o *Orchestrator) JobUpdated(job models.Job) {
	o.jobUpdatedChannel <- job
}

func (o *Orchestrator) sendJobUpdates(ctx context.Context) {
	for jobID, job := range o.updatedJobs {
		childCtx, cancel := context.WithTimeout(ctx, o.config.DataprocManager.RequestTimeout)
		err := o.dataprocManager.UpdateState(childCtx, job)
		cancel()
		if err != nil {
			o.logger.Error(
				fmt.Sprintf("Failed to send job update to dataproc manager: %s", err),
				log.String("JobID", job.ID))
			return
		}
		o.logger.Info("Delivered job update to dataproc manager",
			log.String("JobID", job.ID))
		delete(o.updatedJobs, jobID)
	}
}
