package steps_test

import (
	"context"
	"testing"

	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/dom0discovery"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/dom0discovery/mocks"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/steps"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/types"
	cms_models "a.yandex-team.ru/cloud/mdb/cms/api/internal/models"
	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/models"
	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/opmetas"
	"a.yandex-team.ru/cloud/mdb/internal/juggler"
	jugglermock "a.yandex-team.ru/cloud/mdb/internal/juggler/mocks"
)

func TestDowntimeProlongRun(t *testing.T) {
	type input struct {
		containesOnDom0 []string
		dom0            string
		prepare         func(ctx context.Context, containers []string, dbm *mocks.MockDom0Discovery, jglr *jugglermock.MockAPI)
		metaOnInput     []opmetas.SetDowntimesStepMeta
	}
	type output struct {
		action     steps.AfterStepAction
		forHuman   string
		afterSteps []steps.DecisionStep
	}
	type testCase struct {
		name   string
		input  input
		expect output
	}
	dom0fqdn := "some-dom.db.y.net"
	testCases := []testCase{
		{
			name: "no downtimes to prolong",
			input: input{
				containesOnDom0: []string{"man1.db.y.net", "sas2.db.y.net"},
				dom0:            dom0fqdn,
				prepare: func(
					ctx context.Context,
					containers []string,
					dbm *mocks.MockDom0Discovery,
					jglr *jugglermock.MockAPI,
				) {
					dbmResult := make([]cms_models.Instance, len(containers))
					for ind, container := range containers {
						dbmResult[ind] = cms_models.Instance{
							FQDN: container,
						}
					}
					dbm.EXPECT().Dom0Instances(gomock.Any(), dom0fqdn).Return(dom0discovery.DiscoveryResult{
						WellKnown: dbmResult,
					}, nil)
				},
				metaOnInput: nil,
			},
			expect: output{
				action:     steps.AfterStepContinue,
				forHuman:   "no downtimes remembered",
				afterSteps: []steps.DecisionStep(nil),
			},
		},
		{
			name: "prolong single downtime",
			input: input{
				containesOnDom0: []string{"man1.db.y.net"},
				dom0:            dom0fqdn,
				prepare: func(
					ctx context.Context,
					containers []string,
					dbm *mocks.MockDom0Discovery,
					jglr *jugglermock.MockAPI,
				) {
					dbmResult := make([]cms_models.Instance, len(containers))
					for ind, container := range containers {
						dbmResult[ind] = cms_models.Instance{
							FQDN: container,
						}
					}

					dbm.EXPECT().Dom0Instances(gomock.Any(), dom0fqdn).Return(dom0discovery.DiscoveryResult{
						WellKnown: dbmResult,
					}, nil)

					IDs := []juggler.DowntimeFilter{{DowntimeID: "1"}}
					filters := []juggler.DowntimeFilter{{DowntimeID: "1", Host: "man1.db.y.net"}}
					jglr.EXPECT().GetDowntimes(gomock.Any(), juggler.Downtime{
						Filters: IDs,
					}).Return([]juggler.Downtime{{Filters: filters}}, nil)
					jglr.EXPECT().SetDowntimes(gomock.Any(), gomock.Any())
				},
				metaOnInput: []opmetas.SetDowntimesStepMeta{{DowntimeIDs: []string{"1"}}},
			},
			expect: output{
				action:     steps.AfterStepContinue,
				forHuman:   "downtimes prolonged",
				afterSteps: []steps.DecisionStep(nil),
			},
		},
		{
			name: "don't prolong downtime for absent container",
			input: input{
				containesOnDom0: []string{"man1.db.y.net", "sas2.db.y.net"},
				dom0:            dom0fqdn,
				prepare: func(
					ctx context.Context,
					containers []string,
					dbm *mocks.MockDom0Discovery,
					jglr *jugglermock.MockAPI,
				) {
					dbmResult := make([]cms_models.Instance, 1)
					dbmResult[0] = cms_models.Instance{
						FQDN: "man1.db.y.net",
					}

					dbm.EXPECT().Dom0Instances(gomock.Any(), dom0fqdn).Return(dom0discovery.DiscoveryResult{
						WellKnown: dbmResult,
					}, nil)

					IDs := []juggler.DowntimeFilter{{DowntimeID: "2"}}
					filters := []juggler.DowntimeFilter{{DowntimeID: "2", Host: "sas2.db.y.net"}}
					jglr.EXPECT().GetDowntimes(gomock.Any(), juggler.Downtime{
						Filters: IDs,
					}).Return([]juggler.Downtime{{Filters: filters}}, nil)
				},
				metaOnInput: []opmetas.SetDowntimesStepMeta{{DowntimeIDs: []string{"1"}}, {DowntimeIDs: []string{"2"}}},
			},
			expect: output{
				action:     steps.AfterStepContinue,
				forHuman:   "downtimes prolonged",
				afterSteps: []steps.DecisionStep(nil),
			},
		},
	}
	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			ctx := context.Background()
			ctrl := gomock.NewController(t)
			defer ctrl.Finish()
			dom0d := mocks.NewMockDom0Discovery(ctrl)
			jglr := jugglermock.NewMockAPI(ctrl)
			tc.input.prepare(ctx, tc.input.containesOnDom0, dom0d, jglr)

			step := steps.NewProlongDowntimesStep(jglr, dom0d)
			insCtx := steps.NewEmptyInstructionCtx()

			opsMetaLog := &models.OpsMetaLog{
				Downtimes: tc.input.metaOnInput,
			}

			insCtx.SetActualRD(&types.RequestDecisionTuple{
				R: models.ManagementRequest{Fqnds: []string{dom0fqdn}},
				D: models.AutomaticDecision{OpsLog: opsMetaLog},
			})
			result := step.RunStep(ctx, &insCtx)
			require.NoError(t, result.Error)
			require.Contains(t, result.ForHuman, tc.expect.forHuman)
			require.Equal(t, tc.expect.action, result.Action)
			require.Equal(t, tc.expect.afterSteps, result.AfterMeSteps)
		})
	}
}
