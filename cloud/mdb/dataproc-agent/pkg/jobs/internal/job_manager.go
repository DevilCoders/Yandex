package internal

import (
	"bytes"
	"context"
	"fmt"
	"io"
	"os/exec"
	"strings"
	"sync"
	"time"

	jobconfig "a.yandex-team.ru/cloud/mdb/dataproc-agent/pkg/jobs/config"
	"a.yandex-team.ru/cloud/mdb/dataproc-agent/pkg/models"
	"a.yandex-team.ru/cloud/mdb/dataproc-agent/pkg/yarn"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

//go:generate ../../../../scripts/mockgen.sh JobRunner,OrchestratorAPI,OutputSaver,FSJobOutputSaver

type OrchestratorAPI interface {
	JobUpdated(models.Job)
}

// OutputSaver is an interface for storing logs data to storage
type OutputSaver interface {
	// ReaderFrom blocks until all data have been read from reader
	// See documentation for exec.Cmd#StdoutPipe
	io.ReaderFrom
	Close(context.Context) error
	Flush(context.Context) error
	Disable()
}

// FSJobOutputSaver is an interface for getting a reader to write to FS in addition to the given reader
type FSJobOutputSaver interface {
	SetApplicationID(applicationID string)
	GetReader(stdout io.Reader) io.Reader
	Write(text string) error
	MoveJobLog()
	Close()
}

// JobManager is responsible for running and monitoring job's driver process.
//
// JobManager's algorithms is:
// * search application via yarn api
// * if application is not found, it means that job haven't been run yet or will not start anyway
// * run driver process
// * wait until driver exits; periodically request application's status via yarn api
// * как только будем что-то знать - сообщаем в dataproc-manager
// * as soon as status changes send new status via callback given by OrchestratorAPI
type JobManager struct {
	orchestrator     OrchestratorAPI
	config           jobconfig.Config
	logger           log.Logger
	job              models.Job
	jobRunner        JobRunner
	yarnClient       yarn.API
	s3OutputSaver    OutputSaver
	fsJobOutputSaver FSJobOutputSaver
	cancelOnce       sync.Once
	jobInfoReloader  *time.Ticker
}

func NewJobManager(
	o OrchestratorAPI,
	config jobconfig.Config,
	logger log.Logger,
	job models.Job,
	jobRunner JobRunner,
	yarnClient yarn.API,
	s3OutputSaver OutputSaver,
	fsJobOutputSaver FSJobOutputSaver,
) *JobManager {
	return &JobManager{
		orchestrator:     o,
		config:           config,
		logger:           logger,
		job:              job,
		jobRunner:        jobRunner,
		yarnClient:       yarnClient,
		s3OutputSaver:    s3OutputSaver,
		fsJobOutputSaver: fsJobOutputSaver,
	}
}

func (jm *JobManager) Run(ctx context.Context) {
	jm.logger.Infof("Starting job %s", jm.job.ID)
	defer jm.closeOutputSaver(ctx)
	completionStatusChannel := make(chan error)
	application, _ := jm.getYarnApplication(ctx, true)
	if application == nil {
		go func() {
			err := jm.runJobAndSaveOutput(ctx)
			if err != nil {
				jm.logger.Errorf("Job failed or cancelled: %s", err)
			}
			completionStatusChannel <- err
		}()
	}

	if jm.jobInfoReloader == nil {
		jm.jobInfoReloader = time.NewTicker(jm.config.Yarn.CheckJobStatusInterval)
		defer jm.jobInfoReloader.Stop()
	}

	for !jm.job.State.IsTerminal() {
		select {
		case err := <-completionStatusChannel:
			jm.handleDriverExit(ctx, err == nil)
		case <-jm.jobInfoReloader.C:
			jm.periodicJobCheck(ctx, application != nil)
		case <-ctx.Done():
			return
		}
	}

	jm.logger.Info("Job finished")
}

func (jm *JobManager) doCancel(ctx context.Context) {
	for {
		jm.logger.Infof("job in state %s", jm.job.State)
		if jm.job.Application.ID == "" {
			jm.logger.Infof("Delay job cancellation until yarn application exists")
			time.Sleep(5 * time.Second)
			continue
		}
		jm.logger.Infof("Killing yarn application %s", jm.job.Application.ID)
		_, stderr, err := runCommand("yarn", "application", "-kill", jm.job.Application.ID)
		if err != nil {
			jm.logger.Error(stderr)
		}
		updatedJob, err := jm.actualizeJobWithinYarn(ctx)
		if err != nil {
			return
		}
		if !updatedJob.State.IsTerminal() {
			jm.logger.Infof("Retry job cancellation")
			time.Sleep(5 * time.Second)
			continue
		} else {
			return
		}
	}
}

func (jm *JobManager) Cancel(ctx context.Context) {
	jm.cancelOnce.Do(func() {
		go func() {
			jm.doCancel(ctx)
		}()
	})
}

func runCommand(command string, args ...string) (string, string, error) {
	var stderr bytes.Buffer
	commandParts := append([]string{}, args...)
	cmd := exec.Command(
		command, commandParts...,
	)
	cmd.Stderr = &stderr
	output, err := cmd.Output()
	return string(output), stderr.String(), err
}

func (jm *JobManager) handleDriverExit(ctx context.Context, success bool) {
	updatedJob, err := jm.actualizeJobWithinYarn(ctx)
	if updatedJob.Application.ID != "" {
		jm.fsJobOutputSaver.SetApplicationID(updatedJob.Application.ID)
	} else {
		jm.fsJobOutputSaver.MoveJobLog()
	}
	if err != nil {
		updatedJob = jm.job
	}

	if !updatedJob.State.IsTerminal() || jm.jobRunner.DriverExitCodeDeterminesTerminalState() {
		if success {
			updatedJob.State = models.JobStateDone
		} else {
			updatedJob.State = models.JobStateError
		}
	}

	jm.reportJobUpdate(updatedJob)
}

func (jm *JobManager) periodicJobCheck(ctx context.Context, reportTerminalState bool) {
	updatedJob, err := jm.actualizeJobWithinYarn(ctx)
	if err != nil {
		return
	}
	jm.fsJobOutputSaver.SetApplicationID(updatedJob.Application.ID)

	if jobHasChanges(jm.job, updatedJob) && updatedJob.State.IsTerminal() && !reportTerminalState {
		jm.logger.Infof("New state: %s. Will not be reported because driver process is still running.",
			updatedJob.State)
		return
	}

	jm.reportJobUpdate(updatedJob)
}

func (jm *JobManager) reportJobUpdate(updatedJob models.Job) {
	if jobHasChanges(jm.job, updatedJob) {
		jm.logger.Infof("New state: %s", updatedJob.State)
		jm.orchestrator.JobUpdated(updatedJob)
		jm.job = updatedJob
	}
}

func (jm *JobManager) runJobAndSaveOutput(ctx context.Context) error {

	firstString := fmt.Sprintf("Init job %s at %s\n", jm.job.ID, time.Now().Format(time.UnixDate))
	_, err := jm.s3OutputSaver.ReadFrom(strings.NewReader(firstString))
	if err != nil {
		return xerrors.Errorf("failed to read header line: %w", err)
	}

	childCtx, cancel := context.WithTimeout(ctx, jm.config.S3Logger.FlushTimeout)
	err = jm.s3OutputSaver.Flush(childCtx)
	cancel()
	if err != nil {
		err = jm.fsJobOutputSaver.Write(
			fmt.Sprintf(
				"Can not save logs to specified bucket: %s. "+
					"Check that specified service account can write to this bucket.\n",
				jm.config.S3Logger.BucketName,
			),
		)
		// avoid additional tries to chunk upload for this job
		jm.s3OutputSaver.Disable()
		if err != nil {
			jm.logger.Errorf("File write error: %s", err)
		}
	}

	stdout, ch := jm.runJob()

	// returns T-reader (writes stdout to both file and S3) if JobOutputDirectory is set in config
	// or just the same stdout reader if JobOutputDirectory is not set
	reader := jm.fsJobOutputSaver.GetReader(stdout)
	_, err = jm.s3OutputSaver.ReadFrom(reader)
	if err != nil {
		jm.logger.Errorf("Error reading from stdout: %s", err)
	}
	resultError := <-ch
	message := fmt.Sprintf("Data Proc job finished. Id: %s at %s\n", jm.job.ID, time.Now().Format(time.RFC3339))
	err = jm.fsJobOutputSaver.Write(message)
	if err != nil {
		jm.logger.Errorf("File write error: %s", err)
	}
	jm.closeOutputSaver(ctx)
	return resultError
}

func (jm *JobManager) closeOutputSaver(ctx context.Context) {
	ctx, cancel := context.WithTimeout(ctx, jm.config.S3Logger.FlushTimeout)
	_ = jm.s3OutputSaver.Close(ctx)
	cancel()
	jm.fsJobOutputSaver.Close()
}

func (jm *JobManager) runJob() (io.Reader, chan error) {
	resultChannel := make(chan error, 1)
	err := jm.jobRunner.BeforeRun()
	if err != nil {
		jm.jobRunner.AfterRun()
		resultChannel <- err
		// BeforeRun is expected to return user exposable error
		return strings.NewReader(err.Error() + "\n"), resultChannel
	}

	output, innerResultChannel := jm.jobRunner.Run()

	go func() {
		err = <-innerResultChannel
		jm.jobRunner.AfterRun()
		resultChannel <- err
	}()
	return output, resultChannel
}

func jobHasChanges(oldJobInfo models.Job, newJobInfo models.Job) bool {
	return newJobInfo.State != oldJobInfo.State
}

func (jm *JobManager) actualizeJobWithinYarn(ctx context.Context) (models.Job, error) {
	application, err := jm.getYarnApplication(ctx, false)
	job := jm.job
	if err != nil {
		return models.Job{}, err
	}
	if application != nil {
		job.Application = *application
		job.State = getJobState(*application)
	}
	return job, nil
}

func (jm *JobManager) getYarnApplication(ctx context.Context, repeatOnFailure bool) (app *yarn.Application, err error) {
	for {
		childCtx, cancel := context.WithTimeout(ctx, jm.config.Yarn.RequestTimeout)
		var apps []yarn.Application
		apps, err = jm.yarnClient.SearchApplication(childCtx, jm.jobRunner.YarnTag())
		cancel()
		if err != nil {
			return nil, err
		}
		if len(apps) == 0 {
			return nil, nil
		}
		app, err = jm.jobRunner.SelectYarnApplication(apps)
		if err == nil {
			return
		}
		jm.logger.Errorf("Failed to get app from yarn: %s", err)
		if !repeatOnFailure {
			return
		}
		time.Sleep(jm.config.Yarn.RepeatOnFailureInterval)
	}
}

var internalByYarnState = map[yarn.ApplicationState]models.JobState{
	yarn.StateUnknown:   models.JobStateProvisioning,
	yarn.StateNew:       models.JobStateProvisioning,
	yarn.StateNewSaving: models.JobStateProvisioning,
	yarn.StateSubmitted: models.JobStateProvisioning,
	yarn.StateAccepted:  models.JobStatePending,
	yarn.StateRunning:   models.JobStateRunning,
	yarn.StateFinished:  models.JobStateDone,
	yarn.StateFailed:    models.JobStateError,
	yarn.StateKilled:    models.JobStateCancelled,
}

var internalByYarnFinalStatus = map[yarn.ApplicationFinalStatus]models.JobState{
	yarn.FinalStatusFailed:   models.JobStateError,
	yarn.FinalStatusKilled:   models.JobStateCancelled,
	yarn.FinalStatusSucceded: models.JobStateDone,
}

func getJobState(application yarn.Application) models.JobState {
	internal, found := internalByYarnFinalStatus[application.FinalStatus]
	if found {
		return internal
	}
	internal, found = internalByYarnState[application.State]
	if found {
		return internal
	} else {
		return models.JobStateProvisioning
	}
}
