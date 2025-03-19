package steps_test

import (
	"context"
	"testing"

	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/helpers/juggler"
	stepsmocks "a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/helpers/juggler/mocks"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/steps"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/types"
	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/models"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func TestWaitDom0Reachable(t *testing.T) {
	tcs := []struct {
		name     string
		dom0     string
		expected steps.RunResult
		prepare  func(checker *stepsmocks.MockJugglerChecker)
	}{
		{
			name: "reachable",
			dom0: "fqdn1",
			expected: steps.RunResult{
				ForHuman: "dom0 is reachable",
				Action:   steps.AfterStepContinue,
			},
			prepare: func(checker *stepsmocks.MockJugglerChecker) {
				checker.EXPECT().Check(gomock.Any(), []string{"fqdn1"}, []string{juggler.Unreachable}, gomock.Any()).Return(
					juggler.FQDNGroupByJugglerCheck{OK: []string{"fqdn1"}},
					nil,
				)
			},
		},
		{
			name: "unreachable",
			dom0: "fqdn1",
			expected: steps.RunResult{
				ForHuman: "fqdn1 is unreachable in juggler, you should bring it up",
				Action:   steps.AfterStepWait,
			},
			prepare: func(checker *stepsmocks.MockJugglerChecker) {
				checker.EXPECT().Check(gomock.Any(), []string{"fqdn1"}, []string{juggler.Unreachable}, gomock.Any()).Return(
					juggler.FQDNGroupByJugglerCheck{NotOK: []string{"fqdn1"}},
					nil,
				)
			},
		},
		{
			name: "error",
			dom0: "fqdn1",
			expected: steps.RunResult{
				ForHuman: "error in juggler, will try again",
				Action:   steps.AfterStepWait,
				Error:    xerrors.New("some error"),
			},
			prepare: func(checker *stepsmocks.MockJugglerChecker) {
				checker.EXPECT().Check(gomock.Any(), []string{"fqdn1"}, []string{juggler.Unreachable}, gomock.Any()).Return(
					juggler.FQDNGroupByJugglerCheck{},
					xerrors.New("some error"),
				)
			},
		},
	}
	for _, tc := range tcs {
		t.Run(tc.name, func(t *testing.T) {
			ctrl := gomock.NewController(t)
			defer ctrl.Finish()
			checker := stepsmocks.NewMockJugglerChecker(ctrl)
			tc.prepare(checker)

			opsMetaLog := models.NewOpsMetaLog()
			insCtx := steps.NewEmptyInstructionCtx()
			insCtx.SetActualRD(&types.RequestDecisionTuple{
				R: models.ManagementRequest{Fqnds: []string{tc.dom0}},
				D: models.AutomaticDecision{OpsLog: &opsMetaLog},
			})

			step := steps.NewCustomWaitDom0ReachableStep(checker)
			result := step.RunStep(context.Background(), &insCtx)

			require.Equal(t, tc.expected.ForHuman, result.ForHuman)
			require.Equal(t, tc.expected.Action, result.Action)
			if tc.expected.Error == nil {
				require.NoError(t, result.Error)
			} else {
				require.Equal(t, tc.expected.Error.Error(), result.Error.Error())
			}
		})
	}
}
