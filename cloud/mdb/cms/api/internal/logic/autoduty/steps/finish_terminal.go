package steps

import (
	"context"
	"fmt"

	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/models"
	"a.yandex-team.ru/library/go/slices"
)

// controls that decision in desired state otherwise takes action
type InParticularState struct {
	desiredStates []string
	onOtherStates AfterStepAction
	explainAction string
}

func (s *InParticularState) GetStepName() string {
	return "finally"
}

func (s *InParticularState) RunStep(ctx context.Context, execCtx *InstructionCtx) RunResult {
	rd := execCtx.GetActualRD()
	if slices.ContainsString(s.desiredStates, string(rd.D.Status)) {
		return continueWithMessage(fmt.Sprintf("decision is in state '%s'", rd.D.Status))
	}
	return RunResult{
		ForHuman: fmt.Sprintf(s.explainAction),
		Action:   s.onOtherStates,
	}
}

func NewInParticularState(action AfterStepAction, explainAction string, desiredStates ...models.DecisionStatus) DecisionStep {
	step := InParticularState{
		onOtherStates: action,
		explainAction: explainAction,
	}
	for _, s := range desiredStates {
		step.desiredStates = append(step.desiredStates, string(s))
	}
	return &step
}
