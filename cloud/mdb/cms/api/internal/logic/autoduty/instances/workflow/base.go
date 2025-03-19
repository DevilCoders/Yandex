package workflow

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/cmsdb"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/instances/opcontext"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/instances/steps"
	"a.yandex-team.ru/library/go/core/log"
)

type Letter string

const (
	OKPending     Letter = "ok-pending"
	RejectPending Letter = "reject-pending"
	OK            Letter = "ok"
	Reject        Letter = "reject"
	InProgress    Letter = "in-progress"
)

type Workflow interface {
	Steps() []steps.InstanceStep
	L() log.Logger
	Name() string
	ExecutedStepNames() []string
	AddResult(steps.RunResult)
	StringLog() string
	LastResult() steps.RunResult
	LetterAfterProcessingInProgress() Letter
	LetterAfterCleanupInOKPending() Letter
	LetterAfterCleanupInRejectPending() Letter
	Commit(ctx context.Context, stpctx *opcontext.OperationContext) error

	CleanupSteps() []steps.InstanceStep
	AddCleanupResult(result steps.RunResult)
	LastCleanResult() steps.RunResult
}

type BaseWorkflow struct {
	db          cmsdb.Client
	steps       []steps.InstanceStep
	l           log.Logger
	name        string
	stepResults []steps.RunResult

	cleanupSteps       []steps.InstanceStep
	cleanupStepResults []steps.RunResult
}

func (wf *BaseWorkflow) Steps() []steps.InstanceStep {
	return wf.steps
}

func (wf *BaseWorkflow) ExecutedStepNames() []string {
	stepNames := make([]string, len(wf.stepResults)+len(wf.cleanupStepResults))
	for ind, step := range wf.stepResults {
		stepNames[ind] = step.StepName
	}
	for ind, step := range wf.cleanupStepResults {
		stepNames[len(wf.stepResults)+ind] = step.StepName
	}
	return stepNames
}

func (wf *BaseWorkflow) CleanupSteps() []steps.InstanceStep {
	return wf.cleanupSteps
}

func (wf *BaseWorkflow) L() log.Logger {
	return wf.l
}

func (wf *BaseWorkflow) Name() string {
	return wf.name
}

func (wf *BaseWorkflow) AddResult(result steps.RunResult) {
	wf.stepResults = append(wf.stepResults, result)
}

func (wf *BaseWorkflow) AddCleanupResult(result steps.RunResult) {
	wf.cleanupStepResults = append(wf.cleanupStepResults, result)
}

func NewBaseWorkflow(
	cmsdb cmsdb.Client,
	l log.Logger,
	name string,
	stepList []steps.InstanceStep,
	cleanupStepList []steps.InstanceStep,
) Workflow {
	return &BaseWorkflow{
		steps:        stepList,
		l:            l,
		name:         name,
		db:           cmsdb,
		stepResults:  make([]steps.RunResult, 0),
		cleanupSteps: cleanupStepList,
	}
}

func (wf *BaseWorkflow) LastResult() steps.RunResult {
	if len(wf.stepResults) == 0 {
		return wf.cleanupStepResults[len(wf.cleanupStepResults)-1]
	}
	return wf.stepResults[len(wf.stepResults)-1]
}

func (wf *BaseWorkflow) LastCleanResult() steps.RunResult {
	if len(wf.cleanupStepResults) > 0 {
		return wf.cleanupStepResults[len(wf.cleanupStepResults)-1]
	} else {
		return steps.RunResult{
			IsDone: true,
		}
	}
}

func (wf *BaseWorkflow) LetterAfterCleanupInOKPending() Letter {
	cleanupResult := wf.LastCleanResult()
	if cleanupResult.Error == nil {
		if cleanupResult.IsDone {
			return OK
		}
		return OKPending
	}
	return Reject
}

func (wf *BaseWorkflow) LetterAfterCleanupInRejectPending() Letter {
	cleanupResult := wf.LastCleanResult()
	if cleanupResult.IsDone {
		return Reject
	}
	return RejectPending
}

func (wf *BaseWorkflow) LetterAfterProcessingInProgress() Letter {
	result := wf.LastResult()
	if result.IsDone {
		if result.Error == nil {
			return OKPending
		}
		return RejectPending
	}
	return InProgress
}

func (wf *BaseWorkflow) Commit(ctx context.Context, stpctx *opcontext.OperationContext) error {
	return wf.db.UpdateInstanceOperationFields(ctx, stpctx.CurrentOperation())
}
