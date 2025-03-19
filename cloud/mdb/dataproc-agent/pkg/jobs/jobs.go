package jobs

import (
	"context"
	"fmt"
	"sync/atomic"

	dm "a.yandex-team.ru/cloud/mdb/dataproc-agent/pkg/apiclient"
	jobconfig "a.yandex-team.ru/cloud/mdb/dataproc-agent/pkg/jobs/config"
	"a.yandex-team.ru/cloud/mdb/dataproc-agent/pkg/jobs/internal"
	"a.yandex-team.ru/cloud/mdb/dataproc-agent/pkg/models"
	"a.yandex-team.ru/cloud/mdb/dataproc-agent/pkg/s3logger"
	yarn "a.yandex-team.ru/cloud/mdb/dataproc-agent/pkg/yarn/http"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type Jobs struct {
	config       atomic.Value
	orchestrator *internal.Orchestrator
}

// New initializes jobs component
func New(config jobconfig.Config, dm dm.JobsAPI, getToken models.GetToken, logger log.Logger) *Jobs {
	jobs := Jobs{}
	jobs.config.Store(config)

	yarnClient := &yarn.Client{
		ServerAddress: config.Yarn.ServerAddress,
		Logger:        logger,
	}

	builder := func(o *internal.Orchestrator, job models.Job) (internal.JobManagerAPI, error) {
		currentConfig := jobs.config.Load().(jobconfig.Config)
		taggedLogger := log.With(logger, log.String("JobID", job.ID))

		jobRunner := internal.CreateJobRunner(job, taggedLogger)
		if jobRunner == nil {
			return nil, xerrors.Errorf("unexpected job type: %v", job)
		}

		s3outputSaver, err := newS3Logger(job, currentConfig, getToken, taggedLogger)
		if err != nil {
			logger.Errorf("Failed to create s3 output saver: %s", err)
			return nil, xerrors.Errorf("failed to create s3 output saver: %w", err)
		}
		fsJobOutputSaver := internal.NewFsJobOutputSaver(currentConfig.JobOutputDirectory, job.ID, taggedLogger)
		return internal.NewJobManager(
			o,
			currentConfig,
			taggedLogger,
			job,
			jobRunner,
			yarnClient,
			s3outputSaver,
			fsJobOutputSaver,
		), nil
	}

	jobs.orchestrator = internal.NewOrchestrator(config, dm, builder, logger)

	return &jobs
}

func (jobs *Jobs) Run(ctx context.Context) {
	jobs.orchestrator.Run(ctx)
}

func (jobs *Jobs) SetConfig(config jobconfig.Config) {
	jobs.config.Store(config)
}

func newS3Logger(job models.Job, config jobconfig.Config, getToken models.GetToken,
	logger log.Logger) (internal.OutputSaver, error) {
	if getToken == nil {
		return nil, xerrors.New("S3 connection not initiated")
	}

	filename := fmt.Sprintf("dataproc/clusters/%s/jobs/%s/%s", config.Cid, job.ID, "driveroutput.%09d")
	saver, err := s3logger.New(config.S3Logger, getToken, logger, filename)
	if err != nil {
		return nil, xerrors.Errorf("S3 logger init error: %w", err)
	}

	logger.Infof("Driver output will be saved to s3://%s/%s", config.S3Logger.BucketName, filename)
	return saver, nil
}
