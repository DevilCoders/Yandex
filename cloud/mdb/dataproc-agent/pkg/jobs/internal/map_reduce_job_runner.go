package internal

import (
	"fmt"
	"io"
	"net/url"
	"os"
	"path"
	"sort"
	"strings"

	hadoopfs "a.yandex-team.ru/cloud/mdb/dataproc-agent/pkg/hadoop/fs"
	"a.yandex-team.ru/cloud/mdb/dataproc-agent/pkg/jar"
	"a.yandex-team.ru/cloud/mdb/dataproc-agent/pkg/models"
	"a.yandex-team.ru/cloud/mdb/dataproc-agent/pkg/yarn"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

// MapreduceJobRunner implements execution of map-reduce jobs
type MapreduceJobRunner struct {
	id     string
	logger log.Logger
	models.MapreduceJob
	mainClassFromJar string
	tmpFolder        string
}

func (job *MapreduceJobRunner) YarnTag() string {
	return "dataproc_job_" + job.id
}

func (job *MapreduceJobRunner) Logger() log.Logger {
	return job.logger
}

func (job *MapreduceJobRunner) commandAndArguments() (string, []string) {
	// there're 3 possible ways to run mapreduce job:
	// 1. yarn jar job.jar MainClass genericOptions... args...
	// 2. yarn jar job.jar genericOptions... args...
	// 3. yarn MainClass genericOptions... args...
	// where genericOptions are: -archives, -files, -libjars, -D
	//       args - arguments that are passed directly to mapreduce job class
	// Option 1 is used when main class cannot be extracted from manifest of job.jar,
	// otherwise option 2 is used.

	var arguments []string
	mainClassArgs := job.Args
	if job.MainJarFileURI != "" {
		arguments = append(arguments, "jar", job.MainJarFileURI)

		if job.mainClassFromJar == "org.apache.hadoop.examples.ExampleDriver" && len(job.Args) > 0 {
			// special case: when running command like
			//   yarn jar /usr/lib/hadoop-mapreduce/hadoop-mapreduce-examples.jar pi 1 1000
			// than main class of hadoop-mapreduce-examples.jar - org.apache.hadoop.examples.ExampleDriver -
			// expects that first argument after jar path is subprogram name (like pi, terasort, etc) and
			// then all other arguments (including generic ones) follow
			arguments = append(arguments, job.Args[0])
			mainClassArgs = job.Args[1:]
		}

		if job.mainClassFromJar == "" && len(job.Args) > 0 {
			// if main class is not specified within manifest than first argument after MainJarFileURI
			// should be main class name and we expect that it is given as a first item within job.Args
			arguments = append(arguments, job.Args[0])
			mainClassArgs = job.Args[1:]
		}
	} else {
		arguments = append(arguments, job.MainClass)
	}
	arguments = append(arguments, job.serializeProperties()...)
	arguments = append(arguments, job.serializeGenericOptions()...)
	arguments = append(arguments, mainClassArgs...)
	return "yarn", arguments
}

func (job *MapreduceJobRunner) serializeGenericOptions() []string {
	options := map[string][]string{
		"-archives": job.ArchiveURIs,
		"-files":    job.FileURIs,
		"-libjars":  job.JarFileURIs,
	}
	return serializeURIOptions(options)
}

func (job *MapreduceJobRunner) serializeProperties() []string {
	props := copyProperties(job.Properties)
	addTag(props, "mapreduce.job.tags", job.YarnTag())

	keys := make([]string, 0, len(props))
	for key := range props {
		keys = append(keys, key)
	}
	sort.Strings(keys)

	var arguments []string
	for _, key := range keys {
		value := props[key]
		arguments = append(arguments, fmt.Sprintf("-D%s=%s", key, value))
	}
	return arguments
}

func (job *MapreduceJobRunner) BeforeRun() error {
	job.tmpFolder = path.Join("/tmp", "dataproc-jobs", job.id)
	_ = os.MkdirAll(job.tmpFolder, os.ModePerm)

	if job.MainJarFileURI != "" {
		initialURI := job.MainJarFileURI
		newURI, err := downloadFile(job.MainJarFileURI, job.tmpFolder, job.logger)
		if err != nil {
			return err
		}
		job.MainJarFileURI = newURI

		mainClass, err := jar.MainClass(job.MainJarFileURI)
		if err != nil {
			return xerrors.Errorf("failed to open job JAR %s: %w", initialURI, err)
		}
		job.mainClassFromJar = mainClass
	}

	for idx, uri := range job.JarFileURIs {
		newURI, err := downloadFile(uri, job.tmpFolder, job.logger)
		if err != nil {
			return err
		}
		job.JarFileURIs[idx] = newURI
	}

	return nil
}

func (job *MapreduceJobRunner) AfterRun() {
	err := os.RemoveAll(job.tmpFolder)
	if err != nil {
		job.logger.Errorf("Failed to cleanup after job: %s", err)
	}
}

var copyToLocal = hadoopfs.CopyToLocal

func validateFileURI(sourceURL string) (*url.URL, error) {
	sURL, err := url.Parse(sourceURL)
	if err != nil {
		return sURL, err
	}
	return sURL, nil
}

func downloadFile(sourceURL string, destinationFolder string, logger log.Logger) (string, error) {
	sURL, err := validateFileURI(sourceURL)
	if err != nil {
		return sourceURL, err
	}
	if sURL.Scheme == "" || sURL.Scheme == "file" {
		return sourceURL, nil
	}

	filename := path.Base(sURL.Path)
	localPath := path.Join(destinationFolder, filename)
	logger.Infof("Downloading %s -> %s", sourceURL, localPath)

	err = copyToLocal(sourceURL, localPath)
	if err != nil {
		if !strings.Contains(err.Error(), sourceURL) {
			err = xerrors.Errorf("failed to download file %s: %w", sourceURL, err)
		}
		logger.Infof("%s", err) // this is not our error, that's why we log with info level
		return sourceURL, err
	}

	return localPath, nil
}

func (job *MapreduceJobRunner) Run() (io.Reader, chan error) {
	return RunViaExec(job, job.logger)
}

func (job *MapreduceJobRunner) SelectYarnApplication(apps []yarn.Application) (*yarn.Application, error) {
	return DefaultSelectYarnApplication(job, apps)
}

func (job *MapreduceJobRunner) DriverExitCodeDeterminesTerminalState() bool {
	return false
}
