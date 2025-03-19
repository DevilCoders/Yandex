package internal

import (
	"io"

	"a.yandex-team.ru/cloud/mdb/dataproc-agent/pkg/models"
	"a.yandex-team.ru/cloud/mdb/dataproc-agent/pkg/yarn"
	"a.yandex-team.ru/library/go/core/log"
)

// JobRunner allows to customize execution of jobs of various types.
type JobRunner interface {
	BeforeRun() error
	Run() (io.Reader, chan error)
	AfterRun()
	YarnTag() string
	SelectYarnApplication([]yarn.Application) (*yarn.Application, error)
	Logger() log.Logger
	DriverExitCodeDeterminesTerminalState() bool
}

func CreateJobRunner(job models.Job, logger log.Logger) JobRunner {
	switch jobSpec := job.Spec.(type) {
	case *models.MapreduceJob:
		return &MapreduceJobRunner{
			id:           job.ID,
			logger:       logger,
			MapreduceJob: *jobSpec,
		}
	case *models.SparkJob:
		return &SparkJobRunner{
			id:       job.ID,
			logger:   logger,
			SparkJob: *jobSpec,
		}
	case *models.PysparkJob:
		return &PysparkJobRunner{
			id:         job.ID,
			logger:     logger,
			PysparkJob: *jobSpec,
		}
	case *models.HiveJob:
		return &HiveJobRunner{
			id:      job.ID,
			logger:  logger,
			HiveJob: *jobSpec,
		}
	default:
		return nil
	}
}

// nolint: unparam
func DefaultSelectYarnApplication(jobRunner JobRunner, apps []yarn.Application) (*yarn.Application, error) {
	if len(apps) > 1 {
		jobRunner.Logger().Errorf("Multiple applications found by tag %s", jobRunner.YarnTag())
	}
	if len(apps) > 0 {
		// If everything is ok there'll be only one application with given tag.
		// However if agent fails soon after driver start theoretically it may
		// run driver again after restart.
		// For this case we always return application that was started first.
		first := apps[0]
		for _, app := range apps {
			if app.StartedTime < first.StartedTime {
				first = app
			}
		}
		return &first, nil
	} else {
		return nil, nil
	}
}
