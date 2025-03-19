package grpcclient

import (
	"context"

	dj "a.yandex-team.ru/cloud/bitbucket/public-api/yandex/cloud/dataproc/manager/v1"
	"a.yandex-team.ru/cloud/mdb/dataproc-agent/pkg/apiclient"
	"a.yandex-team.ru/cloud/mdb/dataproc-agent/pkg/models"
)

var externalByInternalState = map[models.JobState]dj.Job_Status{
	models.JobStateProvisioning: dj.Job_PROVISIONING,
	models.JobStatePending:      dj.Job_PENDING,
	models.JobStateRunning:      dj.Job_RUNNING,
	models.JobStateError:        dj.Job_ERROR,
	models.JobStateDone:         dj.Job_DONE,
	models.JobStateCancelled:    dj.Job_CANCELLED,
	models.JobStateCancelling:   dj.Job_CANCELLING,
}

func toExternalState(state models.JobState) dj.Job_Status {
	external, found := externalByInternalState[state]
	if found {
		return external
	} else {
		return dj.Job_STATUS_UNSPECIFIED
	}
}

var internalByExternalState = map[dj.Job_Status]models.JobState{
	dj.Job_PROVISIONING: models.JobStateProvisioning,
	dj.Job_PENDING:      models.JobStatePending,
	dj.Job_RUNNING:      models.JobStateRunning,
	dj.Job_ERROR:        models.JobStateError,
	dj.Job_DONE:         models.JobStateDone,
	dj.Job_CANCELLED:    models.JobStateCancelled,
	dj.Job_CANCELLING:   models.JobStateCancelling,
}

// UpdateState sends updated job state to dataproc manager via grpc
func (c *client) UpdateState(ctx context.Context, job models.Job) error {
	conn, err := c.getConnection(ctx)
	if err != nil {
		return err
	}
	grpcClient := dj.NewJobServiceClient(conn)
	var applicationAttempts []*dj.ApplicationAttempt
	for _, applicationAttempt := range job.Application.ApplicationAttempts {
		applicationAttempts = append(applicationAttempts, &dj.ApplicationAttempt{
			Id:            applicationAttempt.AppAttemptID,
			AmContainerId: applicationAttempt.AMContainerID,
		})
	}
	request := dj.UpdateJobStatusRequest{
		ClusterId: c.cid,
		JobId:     job.ID,
		Status:    toExternalState(job.State),
		ApplicationInfo: &dj.ApplicationInfo{
			Id:                  job.Application.ID,
			ApplicationAttempts: applicationAttempts,
		},
	}
	_, err = grpcClient.UpdateStatus(c.withMeta(ctx), &request)
	return err
}

type JobStream struct {
	client    *client
	jobs      []models.Job
	lastPage  bool
	pageToken string
}

// ListActiveJobs receives unprocessed jobs from dataproc manager via grpc
func (c *client) ListActiveJobs() apiclient.JobStream {
	return &JobStream{client: c}
}

func (s *JobStream) Next(ctx context.Context) (*models.Job, error) {
	if len(s.jobs) == 0 && s.lastPage {
		return nil, nil
	}

	if len(s.jobs) == 0 {
		err := s.loadNextPage(ctx)
		if err != nil {
			return nil, err
		}
	}

	if len(s.jobs) == 0 {
		return nil, nil
	}

	job := s.jobs[0]
	s.jobs = s.jobs[1:]
	return &job, nil
}

func (s *JobStream) loadNextPage(ctx context.Context) error {
	c := s.client
	conn, err := c.getConnection(ctx)
	if err != nil {
		return nil
	}
	grpcClient := dj.NewJobServiceClient(conn)
	request := dj.ListJobsRequest{
		ClusterId: c.cid,
		PageToken: s.pageToken,
	}
	response, err := grpcClient.ListActive(s.client.withMeta(ctx), &request)
	if err != nil {
		return err
	}
	c.logger.Debugf("Received %d jobs from dataproc manager", len(response.Jobs))

	s.pageToken = response.NextPageToken
	if response.NextPageToken == "" {
		s.lastPage = true
	}

	newJobs := make([]models.Job, 0, len(response.Jobs))
	for _, grpcJob := range response.Jobs {
		var job models.Job
		job.ID = grpcJob.Id
		state, found := internalByExternalState[grpcJob.Status]
		if !found {
			c.logger.Errorf("Unexpected job state: %s", grpcJob.Status)
			continue
		}
		job.State = state
		if job.State == models.JobStateCancelling {
			newJobs = append(newJobs, job)
			continue
		}

		switch jobSpec := grpcJob.JobSpec.(type) {
		case *dj.Job_PysparkJob:
			job.Spec = buildPySparkJob(jobSpec.PysparkJob)
		case *dj.Job_SparkJob:
			job.Spec = buildSparkJob(jobSpec.SparkJob)
		case *dj.Job_MapreduceJob:
			job.Spec = buildMapReduceJob(jobSpec.MapreduceJob)
		case *dj.Job_HiveJob:
			job.Spec = buildHiveJob(jobSpec.HiveJob)
		default:
			c.logger.Errorf("Unexpected job type: %s", grpcJob.JobSpec)
			continue
		}

		newJobs = append(newJobs, job)
	}

	s.jobs = newJobs
	return nil
}

func buildPySparkJob(pySparkJob *dj.PysparkJob) models.JobSpec {
	return &models.PysparkJob{
		MainPythonFileURI: pySparkJob.MainPythonFileUri,
		PythonFileURIs:    pySparkJob.PythonFileUris,
		JarFileURIs:       pySparkJob.JarFileUris,
		FileURIs:          pySparkJob.FileUris,
		ArchiveURIs:       pySparkJob.ArchiveUris,
		Packages:          pySparkJob.Packages,
		Repositories:      pySparkJob.Repositories,
		ExcludePackages:   pySparkJob.ExcludePackages,
		Properties:        pySparkJob.Properties,
		Args:              pySparkJob.Args,
	}
}

func buildSparkJob(sparkJob *dj.SparkJob) models.JobSpec {
	return &models.SparkJob{
		MainJarFileURI:  sparkJob.MainJarFileUri,
		MainClass:       sparkJob.MainClass,
		JarFileURIs:     sparkJob.JarFileUris,
		FileURIs:        sparkJob.FileUris,
		ArchiveURIs:     sparkJob.ArchiveUris,
		Packages:        sparkJob.Packages,
		Repositories:    sparkJob.Repositories,
		ExcludePackages: sparkJob.ExcludePackages,
		Properties:      sparkJob.Properties,
		Args:            sparkJob.Args,
	}
}

func buildMapReduceJob(job *dj.MapreduceJob) models.JobSpec {
	return &models.MapreduceJob{
		MainJarFileURI: job.GetMainJarFileUri(),
		MainClass:      job.GetMainClass(),
		JarFileURIs:    job.JarFileUris,
		FileURIs:       job.FileUris,
		ArchiveURIs:    job.ArchiveUris,
		Properties:     job.Properties,
		Args:           job.Args,
	}
}

func buildHiveJob(hiveJob *dj.HiveJob) models.JobSpec {
	var queryList []string
	if hiveJob.GetQueryList() != nil {
		queryList = hiveJob.GetQueryList().Queries
	}
	return &models.HiveJob{
		QueryFileURI:      hiveJob.GetQueryFileUri(),
		QueryList:         queryList,
		ContinueOnFailure: hiveJob.ContinueOnFailure,
		ScriptVariables:   hiveJob.ScriptVariables,
		Properties:        hiveJob.Properties,
		JarFileURIs:       hiveJob.JarFileUris,
	}
}
