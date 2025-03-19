package statemachine

import (
	"context"
	"fmt"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/cmsdb"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/steps"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/types"
	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/models"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type SMRulesTC struct {
	At    models.DecisionStatus
	Input Input
}

type Transition struct {
	Target         models.DecisionStatus
	postSideEffect func(ctx context.Context, cmsdb cmsdb.Client, rd *types.RequestDecisionTuple) error
	preSideEffect  func(ctx context.Context, cmsdb cmsdb.Client, rd *types.RequestDecisionTuple) error
}

/**
Transition rules described:
  * [text](https://wiki.yandex-team.ru/MDB/internal/teams/core/Development/cms/Avtomatika/)
*/
var approvedRequestNoReview = Transition{
	Target:         models.DecisionApprove,
	preSideEffect:  SetAutodutyResolution,
	postSideEffect: AnalysedByAutoDuty}
var approvedRequestMustBeReviewed = Transition{
	Target:         models.DecisionEscalate,
	preSideEffect:  SetAutodutyResolution,
	postSideEffect: AnalysedByAutoDuty}

// build cartesian product based on Input.MustReview flag.
func CartesianProductForMissingCombinations(options map[SMRulesTC]Transition) map[SMRulesTC]Transition {
	for source := range options {
		if source.Input.MustReview {
			destination := source
			destination.Input.MustReview = false
			if _, ok := options[destination]; !ok {
				options[destination] = options[source]
			}
		} else {
			destination := source
			destination.Input.MustReview = true
			if _, ok := options[destination]; !ok {
				options[destination] = options[source]
			}
		}
	}
	return options
}

var availableTransitions = CartesianProductForMissingCombinations(map[SMRulesTC]Transition{
	{models.DecisionNone, Input{Action: steps.ProcessMe}}:      {Target: models.DecisionProcessing},
	{models.DecisionNone, Input{Action: steps.AfterStepClean}}: {Target: models.DecisionCleanup},

	{models.DecisionProcessing, Input{Action: steps.AfterStepWait}}:     {Target: models.DecisionWait},
	{models.DecisionProcessing, Input{steps.AfterStepApprove, false}}:   approvedRequestNoReview,
	{models.DecisionProcessing, Input{steps.AfterStepApprove, true}}:    approvedRequestMustBeReviewed,
	{models.DecisionProcessing, Input{Action: steps.AfterStepEscalate}}: {Target: models.DecisionEscalate},
	{models.DecisionProcessing, Input{Action: steps.AfterStepClean}}:    {Target: models.DecisionCleanup},

	{models.DecisionWait, Input{Action: steps.ProcessMe}}:      {Target: models.DecisionProcessing},
	{models.DecisionWait, Input{Action: steps.AfterStepWait}}:  {Target: models.DecisionWait},
	{models.DecisionWait, Input{Action: steps.AfterStepClean}}: {Target: models.DecisionCleanup},

	{models.DecisionEscalate, Input{Action: steps.ProcessMe}}: {Target: models.DecisionProcessing},
	{models.DecisionEscalate,
		Input{Action: steps.AfterStepApprove, MustReview: false}}: approvedRequestNoReview,
	{models.DecisionEscalate, Input{Action: steps.AfterStepClean}}: {Target: models.DecisionCleanup},

	{models.DecisionApprove, Input{Action: steps.AfterStepWait}}:     {Target: models.DecisionApprove},
	{models.DecisionApprove, Input{Action: steps.AfterStepEscalate}}: {Target: models.DecisionEscalate},
	{models.DecisionApprove, Input{Action: steps.AfterStepAtWalle}}:  {Target: models.DecisionAtWalle, postSideEffect: LetGoByAutoDuty},
	{models.DecisionApprove, Input{Action: steps.AfterStepClean}}:    {Target: models.DecisionCleanup},

	{models.DecisionAtWalle, Input{Action: steps.AfterStepWait}}:     {Target: models.DecisionAtWalle},
	{models.DecisionAtWalle, Input{Action: steps.AfterStepContinue}}: {Target: models.DecisionAtWalle},
	{models.DecisionAtWalle, Input{Action: steps.AfterStepClean}}:    {Target: models.DecisionBeforeDone},

	{models.DecisionBeforeDone, Input{Action: steps.AfterStepWait}}:  {Target: models.DecisionBeforeDone},
	{models.DecisionBeforeDone, Input{Action: steps.AfterStepNext}}:  {Target: models.DecisionCleanup},
	{models.DecisionBeforeDone, Input{Action: steps.AfterStepClean}}: {Target: models.DecisionCleanup},

	{models.DecisionCleanup, Input{Action: steps.AfterStepWait}}:  {Target: models.DecisionCleanup},
	{models.DecisionCleanup, Input{Action: steps.AfterStepClean}}: {Target: models.DecisionCleanup},
	{models.DecisionCleanup, Input{Action: steps.AfterStepNext}}:  {Target: models.DecisionDone, postSideEffect: FinishByAutoDuty},

	// sometimes wall-e deletes tasks are already cleaned by human duty
	{models.DecisionDone, Input{Action: steps.AfterStepClean}}: {Target: models.DecisionDone},
})

var ImpossibleTransitionErr = xerrors.NewSentinel("Impossible transition")

func (sm *StateMachine) TransitToState(ctx context.Context, rd *types.RequestDecisionTuple, input Input) error {
	candidate := SMRulesTC{
		rd.D.Status,
		input,
	}
	t, ok := availableTransitions[candidate]
	if !ok {
		return ImpossibleTransitionErr.Wrap(xerrors.Errorf("in state %q got %v", rd.D.Status, candidate.Input))
	}
	if t.preSideEffect != nil {
		if err := t.preSideEffect(ctx, sm.cmsdb, rd); err != nil {
			return err
		}
	}
	if err := sm.cmsdb.MoveDecisionsToStatus(ctx, []int64{rd.D.ID}, t.Target); err != nil {
		return err
	}
	if t.postSideEffect != nil {
		if err := t.postSideEffect(ctx, sm.cmsdb, rd); err != nil {
			return err
		}
	}
	rd.D.Status = t.Target

	sm.logTransition(ctx, rd.D)
	return nil
}

// log nicely fact of transition to terminal state
func (sm *StateMachine) logTransition(ctx context.Context, d models.AutomaticDecision) {
	ctxlog.Info(
		ctx,
		sm.log,
		fmt.Sprintf("Successful transition in status '%s'", d.Status))
}
