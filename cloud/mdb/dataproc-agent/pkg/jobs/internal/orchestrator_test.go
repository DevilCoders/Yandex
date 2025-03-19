package internal

import (
	"context"
	"testing"
	"time"

	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/dataproc-agent/pkg/apiclient/mocks"
	jobconfig "a.yandex-team.ru/cloud/mdb/dataproc-agent/pkg/jobs/config"
	"a.yandex-team.ru/cloud/mdb/dataproc-agent/pkg/models"
	"a.yandex-team.ru/library/go/core/log/nop"
)

type mockJobManager struct {
	started  bool
	finished bool
	job      models.Job
}

func (r *mockJobManager) Run(_ context.Context) {
	r.started = true
}

func (r *mockJobManager) Cancel(_ context.Context) {
	r.finished = true
}

func (r *mockJobManager) doCancel(_ context.Context) {
	r.finished = true
}

type mockJobManagerFactory struct {
	jobManagers []*mockJobManager
}

func (f *mockJobManagerFactory) newJobManager(o *Orchestrator, job models.Job) (JobManagerAPI, error) {
	jobManager := &mockJobManager{job: job}
	f.jobManagers = append(f.jobManagers, jobManager)
	return jobManager, nil
}

func (f *mockJobManagerFactory) waitForAllJobManagersToExit(t *testing.T, jobManagerExitedChannel chan string) {
	ticker := time.NewTicker(5 * time.Millisecond)
	defer ticker.Stop()

	allFinished := false
	for !allFinished {
		select {
		case <-ticker.C:
			assert.Fail(t, "Timed out")
		case jobID := <-jobManagerExitedChannel:
			allFinished = f.markJobAsFinished(jobID)
		}
	}
}

func (f *mockJobManagerFactory) markJobAsFinished(jobID string) (allFinished bool) {
	for _, jobManager := range f.jobManagers {
		if jobManager.job.ID == jobID {
			jobManager.finished = true
		}
	}
	allFinished = true
	for _, jobManager := range f.jobManagers {
		if !jobManager.finished {
			allFinished = false
			return
		}
	}
	return
}

func TestOrchestratorGetsNewJobs(t *testing.T) {
	ctrl := gomock.NewController(t)

	jobStream := mocks.NewMockJobStream(ctrl)
	jobStream.EXPECT().Next(gomock.Any()).Return(
		&models.Job{JobInfo: models.JobInfo{ID: "1"}, Spec: &models.MapreduceJob{}},
		nil)
	jobStream.EXPECT().Next(gomock.Any()).Return(
		&models.Job{JobInfo: models.JobInfo{ID: "2"}, Spec: &models.SparkJob{}},
		nil)
	jobStream.EXPECT().Next(gomock.Any()).Return(nil, nil)

	dmClient := mocks.NewMockJobsAPI(ctrl)
	dmClient.EXPECT().ListActiveJobs().Return(jobStream)

	factory := mockJobManagerFactory{}
	o := NewOrchestrator(
		jobconfig.Config{MaxConcurrentJobs: 2, MinFreeMemoryToEnqueueNewJob: 0},
		dmClient,
		factory.newJobManager,
		&nop.Logger{},
	)
	o.getNewJobs(context.Background())
	factory.waitForAllJobManagersToExit(t, o.jobManagerExitedChannel)

	require.Equal(t, 2, len(factory.jobManagers))
	require.Equal(t, 2, len(o.jobManagers))
	for _, jobManager := range factory.jobManagers {
		require.Equal(t, true, jobManager.started)
		require.Equal(t, true, jobManager.finished)
	}
}

func TestOrchestratorIsEnoughResources(t *testing.T) {
	ctrl := gomock.NewController(t)

	dmClient := mocks.NewMockJobsAPI(ctrl)

	factory := mockJobManagerFactory{}
	o := NewOrchestrator(
		jobconfig.Config{MaxConcurrentJobs: 2, MinFreeMemoryToEnqueueNewJob: 1099511627776},
		dmClient,
		factory.newJobManager,
		&nop.Logger{},
	)
	require.False(t, o.isEnoughResources())

	o = NewOrchestrator(
		jobconfig.Config{MaxConcurrentJobs: 1, MinFreeMemoryToEnqueueNewJob: 0},
		dmClient,
		factory.newJobManager,
		&nop.Logger{},
	)
	job1, _ := factory.newJobManager(o, models.Job{JobInfo: models.JobInfo{ID: "1"}, Spec: &models.SparkJob{}})
	job2, _ := factory.newJobManager(o, models.Job{JobInfo: models.JobInfo{ID: "2"}, Spec: &models.SparkJob{}})
	jobManagers := map[string]JobManagerAPI{"job1": job1, "job2": job2}

	o.jobManagers = jobManagers
	require.False(t, o.isEnoughResources())
}
