package steps

import (
	"fmt"

	"a.yandex-team.ru/library/go/core/xerrors"
)

func waitWithErrAndMessage(err error, msg string) RunResult {
	result := RunResult{}
	result.AddLog(msg)
	result.Error = err
	return result
}

func waitWithErrAndMessageFmt(err error, msg string, args ...interface{}) RunResult {
	return waitWithErrAndMessage(err, fmt.Sprintf(msg, args...))
}

func waitWithMessage(msg string) RunResult {
	return waitWithErrAndMessage(nil, msg)
}

func waitWithMessageFmt(msg string, args ...interface{}) RunResult {
	return waitWithMessage(fmt.Sprintf(msg, args...))
}

func continueWithMessage(msg string) RunResult {
	result := RunResult{}
	result.AddLog(msg)
	result.IsDone = true
	return result
}

func continueWithMessageFmt(msg string, args ...interface{}) RunResult {
	return continueWithMessage(fmt.Sprintf(msg, args...))
}

func failWithErrorAndMessage(err error, msg string) RunResult {
	result := RunResult{}
	result.AddLog(msg)
	result.IsDone = true
	result.Error = err
	return result
}

func failWithErrorAndMessageFmt(err error, msg string, args ...interface{}) RunResult {
	return failWithErrorAndMessage(err, fmt.Sprintf(msg, args...))
}

func failWithMessage(msg string) RunResult {
	return failWithErrorAndMessage(xerrors.New(msg), msg)
}

func failWithMessageFmt(msg string, args ...interface{}) RunResult {
	return failWithMessage(fmt.Sprintf(msg, args...))
}

func finishWorkflowWithMessage(msg string, args ...interface{}) RunResult {
	result := RunResult{FinishWorkflow: true, IsDone: true}
	result.AddLog(fmt.Sprintf(msg, args...))
	return result
}
