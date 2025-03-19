package instructions

import (
	"context"
	"fmt"
	"time"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/steps"
)

type CommonWorkflow struct {
	Name string
	s    []steps.DecisionStep
}

func (wf *CommonWorkflow) GetStepName() string {
	return fmt.Sprintf("workflow '%s' started at", wf.Name)
}

func (wf *CommonWorkflow) RunStep(ctx context.Context, execCtx *steps.InstructionCtx) steps.RunResult {
	return steps.RunResult{
		ForHuman:     time.Now().Format(time.RFC822),
		Action:       steps.AfterStepContinue,
		AfterMeSteps: wf.s,
	}
}

func NewCommonWorkflow(name string, stepsToExec []steps.DecisionStep) *CommonWorkflow {
	return &CommonWorkflow{s: stepsToExec, Name: name}
}
