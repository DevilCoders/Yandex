package internal

import (
	"io"
	"sort"
	"strings"

	"a.yandex-team.ru/cloud/mdb/dataproc-agent/pkg/models"
	"a.yandex-team.ru/cloud/mdb/dataproc-agent/pkg/yarn"
	"a.yandex-team.ru/library/go/core/log"
)

// PysparkJobRunner implements execution of spark job written in python
type PysparkJobRunner struct {
	id     string
	logger log.Logger
	models.PysparkJob
}

func (job *PysparkJobRunner) YarnTag() string {
	return "dataproc_job_" + job.id
}

func (job *PysparkJobRunner) Logger() log.Logger {
	return job.logger
}

func (job *PysparkJobRunner) commandAndArguments() (string, []string) {
	var arguments []string
	arguments = append(arguments, job.serializeOptions()...)
	arguments = append(arguments, job.serializeProperties()...)
	arguments = append(arguments, job.MainPythonFileURI)
	arguments = append(arguments, job.Args...)
	return "spark-submit", arguments
}

func (job *PysparkJobRunner) serializeOptions() []string {
	options := map[string][]string{
		"--archives":         job.ArchiveURIs,
		"--files":            job.FileURIs,
		"--jars":             job.JarFileURIs,
		"--packages":         job.Packages,
		"--repositories":     job.Repositories,
		"--exclude-packages": job.ExcludePackages,
		"--py-files":         job.PythonFileURIs,
	}
	return serializeURIOptions(options)
}

func serializeURIOptions(options map[string][]string) []string {
	var arguments []string
	optionNames := make([]string, 0, len(options))
	for optionName, uris := range options {
		if len(uris) > 0 {
			optionNames = append(optionNames, optionName)
		}
	}
	sort.Strings(optionNames)
	for _, optionName := range optionNames {
		uris := options[optionName]
		optionValue := strings.Join(uris, ",")
		arguments = append(arguments, optionName)
		arguments = append(arguments, optionValue)
	}
	return arguments
}

func (job *PysparkJobRunner) serializeProperties() []string {
	return serializeSparkSubmitProperties(job.Properties, job)
}

func serializeSparkSubmitProperties(properties map[string]string, jobRunner JobRunner) []string {
	props := copyProperties(properties)
	addTag(props, "spark.yarn.tags", jobRunner.YarnTag())
	return serializeKeyValuePairs("--conf", props)
}

func copyProperties(orig map[string]string) map[string]string {
	clone := make(map[string]string)
	for key, value := range orig {
		clone[key] = value
	}
	return clone
}

func addTag(props map[string]string, tagKey string, tagValue string) {
	tags, found := props[tagKey]
	if found {
		tags = tags + ","
	}
	tags += tagValue
	props[tagKey] = tags
}

func serializeKeyValuePairs(argName string, props map[string]string) []string {
	keys := make([]string, 0, len(props))
	for key := range props {
		keys = append(keys, key)
	}
	sort.Strings(keys)
	var arguments []string
	for _, key := range keys {
		value := props[key]
		arguments = append(arguments, argName)
		arguments = append(arguments, key+"="+value)
	}
	return arguments
}

func (job *PysparkJobRunner) BeforeRun() error {
	if job.Properties == nil {
		job.Properties = make(map[string]string)
	}
	if _, ok := job.Properties["spark.submit.deployMode"]; !ok {
		job.Properties["spark.submit.deployMode"] = "cluster"
	}
	return nil
}

func (job *PysparkJobRunner) Run() (io.Reader, chan error) {
	return RunViaExec(job, job.logger)
}

func (job *PysparkJobRunner) AfterRun() {
}

func (job *PysparkJobRunner) SelectYarnApplication(apps []yarn.Application) (*yarn.Application, error) {
	return DefaultSelectYarnApplication(job, apps)
}

func (job *PysparkJobRunner) DriverExitCodeDeterminesTerminalState() bool {
	return job.Properties["spark.submit.deployMode"] == "client"
}
