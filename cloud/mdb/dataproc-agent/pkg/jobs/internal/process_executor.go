package internal

import (
	"bytes"
	"fmt"
	"io"
	"os/exec"
	"strings"

	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type RunnableViaExec interface {
	commandAndArguments() (command string, arguments []string)
}

func buildCommandLine(job RunnableViaExec) string {
	command, arguments := job.commandAndArguments()
	return command + " " + strings.Join(arguments, " ")
}

func RunViaExec(job RunnableViaExec, logger log.Logger) (io.Reader, chan error) {
	command, arguments := job.commandAndArguments()
	logger.Infof("Starting driver process: %s", buildCommandLine(job))
	cmd := exec.Command(command, arguments...)

	stdout, _ := cmd.StdoutPipe()
	cmd.Stderr = cmd.Stdout
	err := cmd.Start()
	resultChannel := make(chan error, 1)
	if err != nil {
		resultChannel <- xerrors.Errorf("failed to start driver process: %s", err)
		return bytes.NewBufferString(""), resultChannel
	} else {
		pipeReader, pipeWriter := io.Pipe()
		go func() {
			// we have to wait until all output will be read out before calling cmd.Wait
			_, err := io.Copy(pipeWriter, stdout)
			if err != nil {
				logger.Errorf("Failed to copy process output to pipe: %s", err)
			}
			err = pipeWriter.Close()
			if err != nil {
				logger.Errorf("Failed to close pipe writer: %s", err)
			}
			err = cmd.Wait()
			if err != nil {
				logger.Errorf("Subprocess wait returned error: %s", err)
			}
			code := cmd.ProcessState.ExitCode()
			msg := fmt.Sprintf("Driver process has exited with code %d", code)
			if code == 0 {
				logger.Info(msg)
				resultChannel <- nil
			} else {
				resultChannel <- xerrors.New(msg)
			}
		}()

		return pipeReader, resultChannel
	}
}
