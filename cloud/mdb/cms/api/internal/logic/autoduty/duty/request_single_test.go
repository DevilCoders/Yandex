package duty_test

import (
	"context"
	"fmt"
	"testing"

	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/cmsdb/mocks"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/duty"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/instructions"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/steps"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/types"
	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/models"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/cloud/mdb/internal/testutil"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
)

type CommonWorkflow struct {
	afterStep steps.AfterStepAction
}

func (s CommonWorkflow) RunStep(ctx context.Context, execCtx *steps.InstructionCtx) steps.RunResult {
	return steps.RunResult{
		ForHuman: "testing flow",
		Action:   s.afterStep,
	}
}
func (s CommonWorkflow) GetStepName() string {
	return "common workflow"
}

type InData struct {
	wf         steps.DecisionStep
	mustReview bool
	rd         *types.RequestDecisionTuple
}

type ExpectedOut struct {
	ds models.DecisionStatus
}

type TestData struct {
	name string
	in   InData
	exp  ExpectedOut
}

func happyPathsTransition() []TestData {
	var testCases []TestData
	// happy paths

	// processing move to terminal states
	var d = models.AutomaticDecision{
		Status: models.DecisionProcessing,
	}
	testCases = append(testCases, TestData{
		name: fmt.Sprintf("decision '%s' moves from '%s' to '%s'",
			steps.AfterStepEscalate,
			d.Status,
			models.DecisionEscalate),
		in: InData{
			wf: CommonWorkflow{afterStep: steps.AfterStepEscalate},
			rd: &types.RequestDecisionTuple{D: d},
		},
		exp: ExpectedOut{
			ds: models.DecisionEscalate,
		},
	})

	d = models.AutomaticDecision{
		Status: models.DecisionProcessing,
	}
	testCases = append(testCases, TestData{
		name: fmt.Sprintf("decision '%s' moves from '%s' to '%s'",
			steps.AfterStepApprove,
			d.Status,
			models.DecisionApprove),
		in: InData{
			wf: CommonWorkflow{afterStep: steps.AfterStepApprove},
			rd: &types.RequestDecisionTuple{D: d},
		},
		exp: ExpectedOut{
			ds: models.DecisionApprove,
		},
	})

	// after step "continue" does not change state
	for _, status := range []models.DecisionStatus{
		models.DecisionProcessing,
		models.DecisionAtWalle,
		models.DecisionApprove,
		models.DecisionReject,
	} {
		d = models.AutomaticDecision{
			Status: status,
		}
		testCases = append(testCases, TestData{
			name: fmt.Sprintf("decision '%s' not changes state of '%s'",
				steps.AfterStepContinue,
				status),
			in: InData{
				wf: CommonWorkflow{afterStep: steps.AfterStepContinue},
				rd: &types.RequestDecisionTuple{D: d},
			},
			exp: ExpectedOut{
				ds: status,
			},
		})
	}

	for _, status := range []models.DecisionStatus{
		models.DecisionNone,
		models.DecisionWait,
	} {
		d = models.AutomaticDecision{
			Status: status,
		}
		testCases = append(testCases, TestData{
			name: fmt.Sprintf("decision '%s' moves to '%s'",
				status,
				models.DecisionProcessing),
			in: InData{
				wf: CommonWorkflow{afterStep: steps.AfterStepContinue},
				rd: &types.RequestDecisionTuple{D: d},
			},
			exp: ExpectedOut{
				ds: models.DecisionProcessing,
			},
		})
	}

	// autoduty cannot approve if must review
	// human can always approve
	for _, status := range []models.DecisionStatus{
		models.DecisionProcessing,
		models.DecisionEscalate,
		models.DecisionWait,
		models.DecisionNone,
	} {
		// autoduty
		testCases = append(testCases, TestData{
			name: fmt.Sprintf("must review for %s %s to %s",
				steps.AfterStepApprove,
				status,
				models.DecisionEscalate),
			in: InData{
				mustReview: true,
				wf:         CommonWorkflow{afterStep: steps.AfterStepApprove},
				rd: &types.RequestDecisionTuple{D: models.AutomaticDecision{
					Status: status,
				}},
			},
			exp: ExpectedOut{
				ds: models.DecisionEscalate,
			},
		})
		// no review
		testCases = append(testCases, TestData{
			name: fmt.Sprintf("no review for %s %s to %s",
				steps.AfterStepApprove,
				status,
				models.DecisionApprove),
			in: InData{
				mustReview: false,
				wf:         CommonWorkflow{afterStep: steps.AfterStepApprove},
				rd: &types.RequestDecisionTuple{D: models.AutomaticDecision{
					Status: status,
				}},
			},
			exp: ExpectedOut{
				ds: models.DecisionApprove,
			},
		})
	}

	for _, status := range []models.DecisionStatus{
		models.DecisionProcessing,
		models.DecisionEscalate,
		models.DecisionApprove,
		models.DecisionWait,
		models.DecisionNone,
	} {
		testCases = append(testCases, TestData{
			name: fmt.Sprintf("clean moves to cleanup after %s", status),
			in: InData{
				wf: CommonWorkflow{afterStep: steps.AfterStepClean},
				rd: &types.RequestDecisionTuple{D: models.AutomaticDecision{Status: status}},
			},
			exp: ExpectedOut{
				ds: models.DecisionCleanup,
			},
		})
	}

	testCases = append(testCases, TestData{
		name: "clean moves to before-done after wall-e",
		in: InData{
			wf: CommonWorkflow{afterStep: steps.AfterStepClean},
			rd: &types.RequestDecisionTuple{D: models.AutomaticDecision{Status: models.DecisionAtWalle}},
		},
		exp: ExpectedOut{
			ds: models.DecisionBeforeDone,
		},
	})

	return testCases
}

func getInstructionsForDecide(wf steps.DecisionStep) *instructions.Instructions {
	return &instructions.Instructions{
		Default: &instructions.DecisionStrategy{
			EntryPoint: wf,
		},
	}
}

func getDutyForDecide(t *testing.T, in InData) (duty.AutoDuty, *mocks.MockClient, *gomock.Controller) {
	ctx, matcher := testutil.MatchContext(t, context.Background())
	ctrl := gomock.NewController(t)
	cmsdb := mocks.NewMockClient(ctrl)
	cmsdb.EXPECT().Begin(gomock.Any(), sqlutil.Primary).Return(ctx, nil).Times(1)
	cmsdb.EXPECT().Rollback(matcher).Return(nil).Times(1)
	cmsdb.EXPECT().Commit(matcher).Return(nil).Times(1)
	cmsdb.EXPECT().MoveDecisionsToStatus(matcher, gomock.Any(), gomock.Any()).
		Return(nil).
		AnyTimes()
	cmsdb.EXPECT().MarkRequestsResolvedByAutoDuty(matcher, gomock.Any()).Return(nil).AnyTimes()
	cmsdb.EXPECT().MarkRequestsAnalysedByAutoDuty(matcher, gomock.Any()).Return(nil).AnyTimes()
	cmsdb.EXPECT().SetAutoDutyResolution(matcher, gomock.Any(), gomock.Any()).Return(nil).AnyTimes()
	cmsdb.EXPECT().UpdateDecisionFields(matcher, gomock.Any()).Return(nil).AnyTimes()
	cmsdb.EXPECT().UpdateRequestFields(matcher, gomock.Any()).Return(nil).AnyTimes()
	l, _ := zap.New(zap.KVConfig(log.DebugLevel))
	return duty.NewCustomDuty(l, cmsdb, getInstructionsForDecide(in.wf), nil, nil, nil, nil, in.mustReview, 0), cmsdb, ctrl
}

func TestAutoDutyHappyPaths(t *testing.T) {
	ctx := context.Background()

	for _, tc := range happyPathsTransition() {
		t.Run(tc.name, func(t *testing.T) {
			autoduty, _, ctrl := getDutyForDecide(t, tc.in)
			defer ctrl.Finish()
			instrctx := steps.NewEmptyInstructionCtx()
			instrctx.SetActualRD(tc.in.rd)
			instr := &instructions.Instructions{Default: &instructions.DecisionStrategy{EntryPoint: tc.in.wf}}

			err := autoduty.DecideSingleRequest(
				ctx,
				&instrctx,
				instr,
				func(d *models.AutomaticDecision, log string) {},
				func(_ context.Context, _ *steps.InstructionCtx) (*types.RequestDecisionTuple, error) {
					return tc.in.rd, nil
				},
			)

			require.NoError(t, err)
			require.Equal(t, tc.exp.ds, tc.in.rd.D.Status)
		})
	}

}

func TestEnoughTestsForTransitionHere(t *testing.T) {
	exp := 9
	require.Equal(t, exp, len(models.KnownDecisionMap), "need %d tests for new transition in this file, got %d", exp, len(models.KnownDecisionMap))
}
