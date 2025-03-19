package internal

import (
	"fmt"
	"io"
	"math/rand"
	"os"
	"path"
	"strings"
	"time"

	"a.yandex-team.ru/cloud/mdb/dataproc-agent/pkg/models"
	"a.yandex-team.ru/cloud/mdb/dataproc-agent/pkg/yarn"
	"a.yandex-team.ru/library/go/core/log"
)

// HiveJobRunner implements execution of hive jobs
type HiveJobRunner struct {
	id     string
	logger log.Logger
	models.HiveJob
	queryFilePath string
	tempDirectory string
}

func (job *HiveJobRunner) YarnTag() string {
	return "dataproc_job_" + job.id
}

func (job *HiveJobRunner) Logger() log.Logger {
	return job.logger
}

func (job *HiveJobRunner) commandAndArguments() (string, []string) {
	var arguments []string
	arguments = append(arguments, "-u", "jdbc:hive2://localhost:10000")
	if job.ContinueOnFailure {
		arguments = append(arguments, "--force=true")
	}
	arguments = append(arguments, job.serializeHiveconf()...)
	arguments = append(arguments, job.serializeHivevar()...)
	// working with -e (inline query) and --force works in unexpected way in beeline
	// so inline query is being written to file
	arguments = append(arguments, "-f", job.queryFilePath)
	return "beeline", arguments
}

func (job *HiveJobRunner) serializeHiveconf() []string {
	props := copyProperties(job.Properties)
	props["hive.session.id"] = job.hiveSessionID()
	addTag(props, "tez.application.tags", job.YarnTag())

	return serializeKeyValuePairs("--hiveconf", props)
}

func (job *HiveJobRunner) serializeHivevar() []string {
	return serializeKeyValuePairs("--hivevar", job.ScriptVariables)
}

func (job *HiveJobRunner) SelectYarnApplication(apps []yarn.Application) (*yarn.Application, error) {
	return DefaultSelectYarnApplication(job, apps)
}

func (job *HiveJobRunner) hiveSessionID() string {
	return job.YarnTag()
}

func (job *HiveJobRunner) writeQueryListToFile() (string, error) {
	queryFilePath := path.Join(job.tempDirectory, "query.hql")
	queryFile, err := FSUtil.OpenFile(queryFilePath, os.O_TRUNC|os.O_CREATE|os.O_WRONLY, 0644)
	if err != nil {
		return "", err
	}
	defer queryFile.Close()
	_, err = queryFile.WriteString(strings.Join(job.QueryList, ";\n") + ";\n")
	if err != nil {
		return "", err
	}
	return queryFilePath, nil
}

// makes a new file with lines like "ADD JAR <path>;" in the beginning to support deprecated JarFileURIs field
// additional file with rewriting is used because of downloadFile implementation
// that does not allow writing to existing file
func (job *HiveJobRunner) prependJarDependencies() error {
	queryWithJarsFilePath := path.Join(job.tempDirectory, "query_with_jars.hql")
	queryWithJarsFile, err := FSUtil.OpenFile(queryWithJarsFilePath, os.O_TRUNC|os.O_CREATE|os.O_WRONLY, 0644)
	if err != nil {
		return err
	}
	defer queryWithJarsFile.Close()
	for _, jarFileURI := range job.JarFileURIs {
		_, err = validateFileURI(jarFileURI)
		if err != nil {
			return err
		}
		line := fmt.Sprintf("ADD JAR %s;\n", jarFileURI)
		_, err = queryWithJarsFile.WriteString(line)
		if err != nil {
			return err
		}
	}
	queryFile, err := FSUtil.Open(job.queryFilePath)
	if err != nil {
		return err
	}
	defer queryFile.Close()
	_, err = io.Copy(queryWithJarsFile, queryFile)
	if err != nil {
		return err
	}
	job.queryFilePath = queryWithJarsFilePath
	return nil
}

func appendRandomInt(prefix string) string {
	rand.Seed(time.Now().UnixNano())
	randomInt := 1000000 + rand.Intn(9000000)
	return fmt.Sprintf("%s%d", prefix, randomInt)
}

func (job *HiveJobRunner) BeforeRun() error {
	var err error
	// unfortunately os.MkdirTemp is not supported in afero
	job.tempDirectory = appendRandomInt("/tmp/hive-query-")
	err = FSUtil.MkdirAll(job.tempDirectory, 0755)
	if err != nil {
		return err
	}
	if job.QueryFileURI != "" {
		job.queryFilePath, err = downloadFile(job.QueryFileURI, job.tempDirectory, job.logger)
		if err != nil {
			return err
		}
	} else if job.QueryList != nil {
		job.queryFilePath, err = job.writeQueryListToFile()
		if err != nil {
			return err
		}
	}

	if job.JarFileURIs != nil {
		err = job.prependJarDependencies()
		if err != nil {
			return err
		}
	}

	return nil
}

func (job *HiveJobRunner) Run() (io.Reader, chan error) {
	return RunViaExec(job, job.logger)
}

func (job *HiveJobRunner) AfterRun() {
	err := FSUtil.RemoveAll(job.tempDirectory)
	if err != nil {
		job.logger.Errorf("Failed to cleanup after job: %s", err)
	}
}

func (job *HiveJobRunner) DriverExitCodeDeterminesTerminalState() bool {
	return false
}
