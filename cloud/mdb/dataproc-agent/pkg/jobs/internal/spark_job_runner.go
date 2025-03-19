package internal

import (
	"io"

	"a.yandex-team.ru/cloud/mdb/dataproc-agent/pkg/models"
	"a.yandex-team.ru/cloud/mdb/dataproc-agent/pkg/yarn"
	"a.yandex-team.ru/library/go/core/log"
)

// SparkJobRunner implements execution of spark job written in java/scala
type SparkJobRunner struct {
	id     string
	logger log.Logger
	models.SparkJob
}

func (job *SparkJobRunner) YarnTag() string {
	return "dataproc_job_" + job.id
}

func (job *SparkJobRunner) Logger() log.Logger {
	return job.logger
}

func (job *SparkJobRunner) commandAndArguments() (string, []string) {
	var arguments []string
	if job.MainClass != "" {
		arguments = append(arguments, "--class", job.MainClass)
	}
	arguments = append(arguments, job.serializeOptions()...)
	arguments = append(arguments, job.serializeProperties()...)
	arguments = append(arguments, job.MainJarFileURI)
	arguments = append(arguments, job.Args...)
	return "spark-submit", arguments
}

func (job *SparkJobRunner) serializeOptions() []string {
	options := map[string][]string{
		"--archives":         job.ArchiveURIs,
		"--files":            job.FileURIs,
		"--jars":             job.JarFileURIs,
		"--packages":         job.Packages,
		"--repositories":     job.Repositories,
		"--exclude-packages": job.ExcludePackages,
	}
	return serializeURIOptions(options)
}

func (job *SparkJobRunner) serializeProperties() []string {
	return serializeSparkSubmitProperties(job.Properties, job)
}

func (job *SparkJobRunner) BeforeRun() error {
	if job.Properties == nil {
		job.Properties = make(map[string]string)
	}
	if _, ok := job.Properties["spark.submit.deployMode"]; !ok {
		job.Properties["spark.submit.deployMode"] = "cluster"
	}
	return nil
}

func (job *SparkJobRunner) Run() (io.Reader, chan error) {
	return RunViaExec(job, job.logger)
}

func (job *SparkJobRunner) AfterRun() {
}

func (job *SparkJobRunner) SelectYarnApplication(apps []yarn.Application) (*yarn.Application, error) {
	return DefaultSelectYarnApplication(job, apps)
}

func (job *SparkJobRunner) DriverExitCodeDeterminesTerminalState() bool {
	return job.Properties["spark.submit.deployMode"] == "client"
}
