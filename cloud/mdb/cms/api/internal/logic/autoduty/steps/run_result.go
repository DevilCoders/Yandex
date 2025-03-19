package steps

import "fmt"

type AfterStepAction string

const (
	ProcessMe         AfterStepAction = "process"
	AfterStepApprove  AfterStepAction = "immediate_approve"
	AfterStepEscalate AfterStepAction = "escalate"
	AfterStepContinue AfterStepAction = "continue"
	AfterStepWait     AfterStepAction = "wait"
	AfterStepAtWalle  AfterStepAction = "to-wall-e"
	AfterStepNext     AfterStepAction = "next"
	AfterStepClean    AfterStepAction = "clean"
)

type RunResult struct {
	// what has been done in total, in human readable form
	ForHuman     string
	Action       AfterStepAction
	Error        error
	AfterMeSteps []DecisionStep
}

func escalateWithErrAndMsg(err error, msg string) RunResult {
	return RunResult{
		ForHuman: msg,
		Action:   AfterStepEscalate,
		Error:    err,
	}
}

func escalateWithMessage(msg string) RunResult {
	return escalateWithErrAndMsg(nil, msg)
}

func waitWithMessage(msg string, args ...interface{}) RunResult {
	return RunResult{
		ForHuman: fmt.Sprintf(msg, args...),
		Action:   AfterStepWait,
	}
}

func waitWithErrAndMessage(err error, msg string, args ...interface{}) RunResult {
	return RunResult{
		ForHuman: fmt.Sprintf(msg, args...),
		Action:   AfterStepWait,
		Error:    err,
	}
}

func continueWithMessage(msg string, nextSteps ...DecisionStep) RunResult {
	return RunResult{
		ForHuman:     msg,
		Action:       AfterStepContinue,
		AfterMeSteps: nextSteps,
	}
}

func approveWithMessage(msg string) RunResult {
	return RunResult{
		ForHuman: msg,
		Action:   AfterStepApprove,
	}
}
