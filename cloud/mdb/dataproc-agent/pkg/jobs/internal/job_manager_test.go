package internal

import (
	"bytes"
	"context"
	"io"
	"strings"
	"testing"
	"time"

	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/require"

	jobconfig "a.yandex-team.ru/cloud/mdb/dataproc-agent/pkg/jobs/config"
	"a.yandex-team.ru/cloud/mdb/dataproc-agent/pkg/jobs/internal/mocks"
	"a.yandex-team.ru/cloud/mdb/dataproc-agent/pkg/models"
	"a.yandex-team.ru/cloud/mdb/dataproc-agent/pkg/yarn"
	yarnmocks "a.yandex-team.ru/cloud/mdb/dataproc-agent/pkg/yarn/mocks"
	"a.yandex-team.ru/library/go/core/log/nop"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type contextWithTimeout struct {
	t          *testing.T
	ctx        context.Context
	cancelFunc context.CancelFunc
}

func newContextWithTimeout(t *testing.T) *contextWithTimeout {
	obj := &contextWithTimeout{t: t}
	ctx, cancel := context.WithTimeout(context.Background(), time.Second)
	obj.ctx = ctx
	obj.cancelFunc = cancel
	return obj
}

func (c *contextWithTimeout) checkNoTimeout() {
	c.cancelFunc()
	if c.ctx.Err() == context.DeadlineExceeded {
		c.t.Fatalf("Error: %v", c.ctx.Err())
	}
}

func (c *contextWithTimeout) cancel() {
	c.cancelFunc()
}

func newJobManagerWithMocks(t *testing.T, job models.Job, config jobconfig.Config) (
	*JobManager,
	*mocks.MockOrchestratorAPI,
	*mocks.MockJobRunner,
	*yarnmocks.MockAPI,
	*mocks.MockOutputSaver,
	*mocks.MockFSJobOutputSaver,
) {
	ctrl := gomock.NewController(t)
	logger := &nop.Logger{}
	o := mocks.NewMockOrchestratorAPI(ctrl)
	s3OutputSaver := mocks.NewMockOutputSaver(ctrl)
	fsOutputSaver := mocks.NewMockFSJobOutputSaver(ctrl)
	yarnClient := yarnmocks.NewMockAPI(ctrl)
	jobRunner := mocks.NewMockJobRunner(ctrl)

	jobManager := NewJobManager(o, config, logger, job, jobRunner, yarnClient, s3OutputSaver, fsOutputSaver)

	return jobManager, o, jobRunner, yarnClient, s3OutputSaver, fsOutputSaver
}

func runWithTimeout(t *testing.T, jobManager *JobManager) {
	timer := newContextWithTimeout(t)
	jobManager.Run(timer.ctx)
	timer.checkNoTimeout()
}

func yarnReturnsState(state yarn.ApplicationState, tag string, yarnClient *yarnmocks.MockAPI, jobRunner *mocks.MockJobRunner) yarn.Application {
	app := yarn.Application{State: state}
	yarnClient.EXPECT().SearchApplication(gomock.Any(), tag).Return([]yarn.Application{app}, nil).Times(1)
	jobRunner.EXPECT().SelectYarnApplication([]yarn.Application{app}).Return(&app, nil).Times(1)
	return app
}

func expectHeader(t *testing.T, outputSaver *mocks.MockOutputSaver) {
	flushHeaderWithError(t, outputSaver, nil)
}

func flushHeaderWithError(t *testing.T, outputSaver *mocks.MockOutputSaver, err error) {
	outputSaver.EXPECT().ReadFrom(gomock.Any()).
		DoAndReturn(func(r io.Reader) (int64, error) {
			buffer := bytes.Buffer{}
			n, err := buffer.ReadFrom(r)
			require.NoError(t, err)
			require.Contains(t, buffer.String(), "Init job 777 at")
			return n, err
		}).Times(1)
	outputSaver.EXPECT().Flush(gomock.Any()).Return(err).Times(1)
}

// Scenario: successfully run job
func TestJobManager1(t *testing.T) {
	config := jobconfig.DefaultConfig()
	job := models.Job{JobInfo: models.JobInfo{ID: "777"}}
	jobManager, o, jobRunner, yarnClient, outputSaver, fsOutputSaver := newJobManagerWithMocks(t, job, config)

	tag := "dataproc_job_777"
	jobRunner.EXPECT().YarnTag().Return(tag).MinTimes(1)
	yarnClient.EXPECT().SearchApplication(gomock.Any(), tag).Return([]yarn.Application{}, nil).Times(1)
	expectHeader(t, outputSaver)
	jobRunner.EXPECT().BeforeRun().Return(nil).Times(1)
	jobRunner.EXPECT().AfterRun().Times(1)
	output := strings.NewReader("this is job output")
	ch := make(chan error, 1)
	ch <- nil
	jobRunner.EXPECT().Run().Return(output, ch).Times(1)
	fsOutputSaver.EXPECT().GetReader(output).Return(output).Times(1)
	outputSaver.EXPECT().ReadFrom(output).Return(int64(18), nil).Times(1)
	outputSaver.EXPECT().Close(gomock.Any()).Return(nil).MinTimes(1)
	fsOutputSaver.EXPECT().Write(gomock.Any()).Return(nil).Times(1)
	fsOutputSaver.EXPECT().Close().Return().MinTimes(1)
	fsOutputSaver.EXPECT().MoveJobLog().Return().Times(1)
	app := yarn.Application{State: yarn.StateFinished}
	yarnClient.EXPECT().SearchApplication(gomock.Any(), tag).Return([]yarn.Application{app}, nil).Times(1)
	jobRunner.EXPECT().SelectYarnApplication([]yarn.Application{app}).Return(&app, nil).Times(1)
	jobRunner.EXPECT().DriverExitCodeDeterminesTerminalState().Return(false).Times(1)
	job2 := models.Job{JobInfo: models.JobInfo{ID: "777", State: models.JobStateDone}, Application: app}
	o.EXPECT().JobUpdated(job2).Times(1)

	runWithTimeout(t, jobManager)
}

// Scenario: JobRunner#BeforeRun() returns error
func TestJobManager2(t *testing.T) {
	config := jobconfig.DefaultConfig()
	job := models.Job{JobInfo: models.JobInfo{ID: "777"}}
	jobManager, o, jobRunner, yarnClient, outputSaver, fsOutputSaver := newJobManagerWithMocks(t, job, config)

	tag := "dataproc_job_777"
	jobRunner.EXPECT().YarnTag().Return(tag).MinTimes(1)
	yarnClient.EXPECT().SearchApplication(gomock.Any(), tag).Return([]yarn.Application{}, nil).Times(1)
	expectHeader(t, outputSaver)
	msg := "before run failed"
	jobRunner.EXPECT().BeforeRun().Return(xerrors.New(msg)).Times(1)
	jobRunner.EXPECT().AfterRun().Times(1)
	output := strings.NewReader(msg + "\n")
	fsOutputSaver.EXPECT().GetReader(output).Return(output).Times(1)
	outputSaver.EXPECT().ReadFrom(output).Return(int64(18), nil).Times(1)
	outputSaver.EXPECT().Close(gomock.Any()).Return(nil).MinTimes(1)
	fsOutputSaver.EXPECT().Write(gomock.Any()).Return(nil).Times(1)
	fsOutputSaver.EXPECT().Close().Return().MinTimes(1)
	fsOutputSaver.EXPECT().MoveJobLog().Return().Times(1)
	yarnClient.EXPECT().SearchApplication(gomock.Any(), tag).Return([]yarn.Application{}, nil).Times(1)
	job2 := models.Job{JobInfo: models.JobInfo{ID: "777", State: models.JobStateError}}
	o.EXPECT().JobUpdated(job2).Times(1)

	runWithTimeout(t, jobManager)
}

// Scenario: JobRunner#Run() returns error
func TestJobManager3(t *testing.T) {
	config := jobconfig.DefaultConfig()
	job := models.Job{JobInfo: models.JobInfo{ID: "777"}}
	jobManager, o, jobRunner, yarnClient, outputSaver, fsOutputSaver := newJobManagerWithMocks(t, job, config)

	tag := "dataproc_job_777"
	jobRunner.EXPECT().YarnTag().Return(tag).MinTimes(1)
	yarnClient.EXPECT().SearchApplication(gomock.Any(), tag).Return([]yarn.Application{}, nil).Times(1)
	expectHeader(t, outputSaver)
	jobRunner.EXPECT().BeforeRun().Return(nil).Times(1)
	jobRunner.EXPECT().AfterRun().Times(1)
	output := strings.NewReader("this is job output")
	ch := make(chan error, 1)
	ch <- xerrors.New("failed to execute job")
	jobRunner.EXPECT().Run().Return(output, ch).Times(1)
	fsOutputSaver.EXPECT().GetReader(output).Return(output).Times(1)
	outputSaver.EXPECT().ReadFrom(output).Return(int64(18), nil).Times(1)
	outputSaver.EXPECT().Close(gomock.Any()).Return(nil).MinTimes(1)
	fsOutputSaver.EXPECT().Write(gomock.Any()).Return(nil).Times(1)
	fsOutputSaver.EXPECT().Close().Return().MinTimes(1)
	fsOutputSaver.EXPECT().MoveJobLog().Return().Times(1)
	// emulate that we fail job even if yarn doesn't know about it
	yarnClient.EXPECT().SearchApplication(gomock.Any(), tag).Return([]yarn.Application{}, nil).Times(1)
	job2 := models.Job{JobInfo: models.JobInfo{ID: "777", State: models.JobStateError}}
	o.EXPECT().JobUpdated(job2).Times(1)

	runWithTimeout(t, jobManager)
}

// Scenario: Attach to job after dataproc agent restart
func TestJobManager4(t *testing.T) {
	config := jobconfig.DefaultConfig()
	job := models.Job{JobInfo: models.JobInfo{ID: "777"}}
	jobManager, o, jobRunner, yarnClient, outputSaver, fsOutputSaver := newJobManagerWithMocks(t, job, config)
	reloadJobInfo := make(chan time.Time, 10)
	jobManager.jobInfoReloader = &time.Ticker{C: reloadJobInfo}

	tag := "dataproc_job_777"
	jobRunner.EXPECT().YarnTag().Return(tag).MinTimes(1)

	_ = yarnReturnsState(yarn.StateAccepted, tag, yarnClient, jobRunner)

	reloadJobInfo <- time.Now()
	app := yarnReturnsState(yarn.StateRunning, tag, yarnClient, jobRunner)
	o.EXPECT().JobUpdated(
		models.Job{JobInfo: models.JobInfo{ID: "777", State: models.JobStateRunning}, Application: app}).Times(1)

	reloadJobInfo <- time.Now()
	app = yarnReturnsState(yarn.StateFinished, tag, yarnClient, jobRunner)
	o.EXPECT().JobUpdated(
		models.Job{JobInfo: models.JobInfo{ID: "777", State: models.JobStateDone}, Application: app}).Times(1)
	fsOutputSaver.EXPECT().SetApplicationID(gomock.Any()).Return().MinTimes(1)

	// we'll write nothing to outputSaver in this case, but it should be closed anyway
	outputSaver.EXPECT().Close(gomock.Any()).Return(nil).Times(1)
	fsOutputSaver.EXPECT().Close().Return().MinTimes(1)

	runWithTimeout(t, jobManager)
}

// Scenario: do not report terminal state until job output is saved
// test for https://st.yandex-team.ru/MDB-7456
func TestJobManager5(t *testing.T) {
	config := jobconfig.DefaultConfig()
	job := models.Job{JobInfo: models.JobInfo{ID: "777"}}
	jobManager, o, jobRunner, yarnClient, outputSaver, fsOutputSaver := newJobManagerWithMocks(t, job, config)
	reloadJobInfo := make(chan time.Time, 10)
	jobManager.jobInfoReloader = &time.Ticker{C: reloadJobInfo}

	tag := "dataproc_job_777"
	jobRunner.EXPECT().YarnTag().Return(tag).MinTimes(1)
	yarnClient.EXPECT().SearchApplication(gomock.Any(), tag).Return([]yarn.Application{}, nil).Times(1)
	expectHeader(t, outputSaver)
	jobRunner.EXPECT().BeforeRun().Return(nil).Times(1)
	jobRunner.EXPECT().AfterRun().Times(1)
	output := strings.NewReader("this is job output")
	ch := make(chan error, 1)
	ch <- nil
	jobRunner.EXPECT().Run().Return(output, ch).Times(1)
	fsOutputSaver.EXPECT().GetReader(output).Return(output).Times(1)
	outputSaver.EXPECT().ReadFrom(output).Return(int64(18), nil).Times(1)
	wait := make(chan struct{})
	outputSaver.EXPECT().Close(gomock.Any()).Return(nil).MinTimes(1).
		Do(func(interface{}) { reloadJobInfo <- time.Now(); <-wait })
	fsOutputSaver.EXPECT().Write(gomock.Any()).Return(nil).Times(1)
	fsOutputSaver.EXPECT().SetApplicationID(gomock.Any()).Return().MinTimes(1)

	fsOutputSaver.EXPECT().Close().Return().MinTimes(1)
	fsOutputSaver.EXPECT().MoveJobLog().Return().Times(1)

	// here periodic check (initiated by reloadJobInfo <- time.Now()) starts
	// receives terminal state from yarn, but does not report it to orchestrator
	app := yarn.Application{State: yarn.StateFinished}
	yarnClient.EXPECT().SearchApplication(gomock.Any(), tag).Return([]yarn.Application{app}, nil).Times(1)
	jobRunner.EXPECT().SelectYarnApplication([]yarn.Application{app}).Return(&app, nil).
		Times(1).
		Do(func(interface{}) { close(wait) })
	jobRunner.EXPECT().DriverExitCodeDeterminesTerminalState().Return(false).Times(1)

	// here outputSaver.Close() returns, we recheck state via yarn
	// and report terminal state to orchestrator
	yarnReturnsState(yarn.StateFinished, tag, yarnClient, jobRunner)
	o.EXPECT().JobUpdated(
		models.Job{JobInfo: models.JobInfo{ID: "777", State: models.JobStateDone}, Application: app}).Times(1)

	runWithTimeout(t, jobManager)
}

// Scenario: Error syncing job output's header line to S3
func TestJobManager6(t *testing.T) {
	config := jobconfig.DefaultConfig()
	job := models.Job{JobInfo: models.JobInfo{ID: "777"}}
	jobManager, o, jobRunner, yarnClient, outputSaver, fsOutputSaver := newJobManagerWithMocks(t, job, config)

	tag := "dataproc_job_777"
	jobRunner.EXPECT().YarnTag().Return(tag).MinTimes(1)
	jobRunner.EXPECT().BeforeRun().Return(nil).Times(1)
	jobRunner.EXPECT().AfterRun().Times(1)
	yarnClient.EXPECT().SearchApplication(gomock.Any(), tag).Return([]yarn.Application{}, nil).Times(1)
	flushHeaderWithError(t, outputSaver, xerrors.New("s3 request failed"))
	outputSaver.EXPECT().Disable().Return().Times(1)

	output := strings.NewReader("this is job output")
	ch := make(chan error, 1)
	ch <- nil
	jobRunner.EXPECT().Run().Return(output, ch).Times(1)
	fsOutputSaver.EXPECT().GetReader(output).Return(output).Times(1)
	outputSaver.EXPECT().ReadFrom(output).Return(int64(18), nil).Times(1)

	fsOutputSaver.EXPECT().Write("Can not save logs to specified bucket: missing-bucket. Check that specified service account can write to this bucket.\n").Return(nil).Times(1)
	fsOutputSaver.EXPECT().Write(gomock.Any()).Return(nil).Times(1) // job finished termination string
	fsOutputSaver.EXPECT().Close().Return().MinTimes(1)
	outputSaver.EXPECT().Close(gomock.Any()).Return(nil).MinTimes(1)
	fsOutputSaver.EXPECT().MoveJobLog().Return().Times(1)
	yarnClient.EXPECT().SearchApplication(gomock.Any(), tag).Return([]yarn.Application{}, nil).Times(1)

	job2 := models.Job{JobInfo: models.JobInfo{ID: "777", State: models.JobStateDone}}
	o.EXPECT().JobUpdated(job2).Times(1)

	runWithTimeout(t, jobManager)
}

// Scenario: If property spark.submit.deployMode=client is set for spark and pyspark jobs
//           (and DriverExitCodeDeterminesTerminalState() returns true), than on failure job is marked as Failed even
//           if yarn considers it to be successful.
// Test for https://st.yandex-team.ru/MDB-13986
func TestJobManager7(t *testing.T) {
	config := jobconfig.DefaultConfig()
	job := models.Job{JobInfo: models.JobInfo{ID: "777"}}
	jobManager, o, jobRunner, yarnClient, outputSaver, fsOutputSaver := newJobManagerWithMocks(t, job, config)

	tag := "dataproc_job_777"
	jobRunner.EXPECT().YarnTag().Return(tag).MinTimes(1)
	yarnClient.EXPECT().SearchApplication(gomock.Any(), tag).Return([]yarn.Application{}, nil).Times(1)
	expectHeader(t, outputSaver)
	jobRunner.EXPECT().BeforeRun().Return(nil).Times(1)
	jobRunner.EXPECT().AfterRun().Times(1)
	output := strings.NewReader("this is job output")
	ch := make(chan error, 1)
	ch <- xerrors.New("failed to execute job")
	jobRunner.EXPECT().Run().Return(output, ch).Times(1)
	fsOutputSaver.EXPECT().GetReader(output).Return(output).Times(1)
	outputSaver.EXPECT().ReadFrom(output).Return(int64(18), nil).Times(1)
	outputSaver.EXPECT().Close(gomock.Any()).Return(nil).MinTimes(1)
	fsOutputSaver.EXPECT().Write(gomock.Any()).Return(nil).Times(1)
	fsOutputSaver.EXPECT().Close().Return().MinTimes(1)
	fsOutputSaver.EXPECT().MoveJobLog().Return().Times(1)
	app := yarn.Application{State: yarn.StateFinished}
	yarnClient.EXPECT().SearchApplication(gomock.Any(), tag).Return([]yarn.Application{app}, nil).Times(1)
	jobRunner.EXPECT().SelectYarnApplication([]yarn.Application{app}).Return(&app, nil).Times(1)
	jobRunner.EXPECT().DriverExitCodeDeterminesTerminalState().Return(true).Times(1)
	job2 := models.Job{JobInfo: models.JobInfo{ID: "777", State: models.JobStateError}, Application: app}
	o.EXPECT().JobUpdated(job2).Times(1)

	runWithTimeout(t, jobManager)
}
