package internal

import (
	"strings"
	"testing"

	iowrap "github.com/spf13/afero"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/library/go/core/log/nop"
)

func init() {
	FS = iowrap.NewMemMapFs()
	FSUtil = &iowrap.Afero{Fs: FS}
}

func TestSetApplicationID(t *testing.T) {
	logger := &nop.Logger{}
	_ = FS.MkdirAll("/some/path", 0755)
	_ = iowrap.WriteFile(FS, "/some/path/job_1.log", []byte("some log line\n"), 0644)

	fsJobOutputSaver := NewFsJobOutputSaver(
		"/some/path",
		"job_1",
		logger,
	)
	logLine := "some log line\n"
	jobLogTemporaryFilePath := "/some/path/job_1.log"
	jobLogFilePath := "/some/path/job_job_1_application_1.log"
	stdout := strings.NewReader(logLine)
	reader := fsJobOutputSaver.GetReader(stdout)
	var s3buf []byte
	_, _ = reader.Read(s3buf)

	require.Equal(t, jobLogTemporaryFilePath, fsJobOutputSaver.temporaryJobOutputFilePath)
	require.Equal(t, "", fsJobOutputSaver.resultJobOutputFilePath)

	fsbuf, _ := iowrap.ReadFile(FS, jobLogTemporaryFilePath)
	require.Equal(t, logLine, string(fsbuf))

	fsJobOutputSaver.SetApplicationID("application_1")
	require.Equal(t, jobLogFilePath, fsJobOutputSaver.resultJobOutputFilePath)

	fsbuf, _ = iowrap.ReadFile(FS, jobLogFilePath)
	require.Equal(t, logLine, string(fsbuf))
}
