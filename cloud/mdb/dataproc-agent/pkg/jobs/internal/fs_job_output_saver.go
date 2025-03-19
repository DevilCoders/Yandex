package internal

import (
	"errors"
	"fmt"
	"io"
	"os"

	iowrap "github.com/spf13/afero"

	"a.yandex-team.ru/library/go/core/log"
)

var (
	FS     iowrap.Fs
	FSUtil *iowrap.Afero
)

func init() {
	FS = iowrap.NewOsFs()
	FSUtil = &iowrap.Afero{Fs: FS}
}

type FsJobOutputSaver struct {
	logger                     log.Logger
	jobOutputDirectory         string
	jobID                      string
	applicationID              string
	temporaryJobOutputFilePath string
	resultJobOutputFilePath    string
	jobLogFile                 iowrap.File
}

func NewFsJobOutputSaver(
	jobOutputDirectory string,
	jobID string,
	logger log.Logger,
) *FsJobOutputSaver {
	return &FsJobOutputSaver{
		logger:                     logger,
		jobOutputDirectory:         jobOutputDirectory,
		jobID:                      jobID,
		temporaryJobOutputFilePath: fmt.Sprintf("%s/%s.log", jobOutputDirectory, jobID),
	}
}

func (js *FsJobOutputSaver) SetApplicationID(applicationID string) {
	// the renaming of the job stdout in fs is being done here
	// because YARN application ID is not known at the beginning of the reading of the log
	// and application ID is needed ASAP to start sending growing job log to cloud logging service
	// fluentbit agent will start sending a log when its filename starts with "application_"
	if js.jobOutputDirectory != "" && js.applicationID != applicationID {
		if _, err := FSUtil.Stat(js.temporaryJobOutputFilePath); err == nil {
			js.resultJobOutputFilePath = fmt.Sprintf(
				"%s/job_%s_%s.log",
				js.jobOutputDirectory,
				js.jobID,
				applicationID,
			)
			err = FSUtil.Rename(
				js.temporaryJobOutputFilePath,
				js.resultJobOutputFilePath,
			)
			if err == nil {
				js.applicationID = applicationID
			} else {
				js.logger.Errorf(
					"Error renaming file %s to %s : %s",
					js.temporaryJobOutputFilePath,
					js.resultJobOutputFilePath,
					err,
				)
			}
		}
	}
}

func (js *FsJobOutputSaver) GetReader(stdout io.Reader) io.Reader {
	var err error
	if js.jobOutputDirectory != "" {
		if _, err = FSUtil.Stat(js.jobOutputDirectory); errors.Is(err, os.ErrNotExist) {
			err = FSUtil.MkdirAll(js.jobOutputDirectory, os.ModePerm)
			if err != nil {
				js.logger.Errorf("Error creating directory %s : %s", js.jobOutputDirectory, err)
			}
		}
		js.jobLogFile, err = FSUtil.OpenFile(js.temporaryJobOutputFilePath, os.O_APPEND|os.O_CREATE|os.O_WRONLY, 0644)
		if err != nil {
			js.logger.Errorf("Error opening file %s : %s", js.temporaryJobOutputFilePath, err)
			return stdout
		} else {
			teeReader := io.TeeReader(stdout, js.jobLogFile)
			return teeReader
		}
	} else {
		return stdout
	}
}

func (js *FsJobOutputSaver) Write(text string) error {
	if js.jobOutputDirectory != "" {
		var err error
		if js.jobLogFile == nil {
			js.jobLogFile, err = FSUtil.OpenFile(js.temporaryJobOutputFilePath, os.O_APPEND|os.O_CREATE|os.O_WRONLY, 0644)
			if err != nil {
				return err
			}
		}
		_, err = js.jobLogFile.WriteString(text)
		if err != nil {
			return err
		}
	}
	return nil
}

func (js *FsJobOutputSaver) MoveJobLog() {
	if js.jobOutputDirectory != "" {
		err := FSUtil.Rename(
			js.temporaryJobOutputFilePath,
			fmt.Sprintf("%s/job_%s.log", js.jobOutputDirectory, js.jobID),
		)
		if err != nil {
			js.logger.Errorf(
				"Error renaming file %s to %s : %s",
				js.temporaryJobOutputFilePath,
				js.resultJobOutputFilePath,
				err,
			)
		}
	}
}

func (js *FsJobOutputSaver) Close() {
	if js.jobLogFile != nil {
		err := js.jobLogFile.Close()
		if err != nil {
			js.logger.Errorf(
				"Error closing file %s : %s",
				js.jobLogFile.Name(),
				err,
			)
		}
	}
}
