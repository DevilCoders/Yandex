package workflow_test

import (
	"context"
	"sync"
	"testing"
	"time"

	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/require"
	"golang.org/x/xerrors"

	cmsdbmocks "a.yandex-team.ru/cloud/mdb/cms/api/internal/cmsdb/mocks"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/instances/opcontext"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/instances/steps"
	stepmocks "a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/instances/steps/mocks"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/instances/workflow"
	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/models"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
)

func TestRunWorkflow(t *testing.T) {
	testCases := []struct {
		name               string
		savedOp            models.ManagementInstanceOperation
		stepFactory        func(ctrl *gomock.Controller) []steps.InstanceStep
		cleanupStepFactory func(ctrl *gomock.Controller) []steps.InstanceStep
		opFactory          func() models.ManagementInstanceOperation
		expectedOp         models.ManagementInstanceOperation
	}{
		{
			name: "simple success",
			stepFactory: func(ctrl *gomock.Controller) []steps.InstanceStep {
				step := stepmocks.NewMockInstanceStep(ctrl)
				step.EXPECT().Name().AnyTimes().Return("test step")
				step.EXPECT().RunStep(gomock.Any(), gomock.Any(), gomock.Any()).DoAndReturn(
					func(ctx context.Context, stepCtx *opcontext.OperationContext, l log.Logger) steps.RunResult {
						stepCtx.State().MoveInstanceStep.TaskID = "123qwe"
						res := steps.RunResult{
							Log:         []string{"okay"},
							Description: "it's a decription",
							IsDone:      true,
						}
						return res
					})
				return []steps.InstanceStep{step}
			},
			cleanupStepFactory: func(ctrl *gomock.Controller) []steps.InstanceStep {
				return nil
			},
			opFactory: func() models.ManagementInstanceOperation {
				return models.ManagementInstanceOperation{
					ID:         "op id",
					Type:       models.InstanceOperationMove,
					Status:     models.InstanceOperationStatusNew,
					InstanceID: "fqdn id",
					State:      models.DefaultOperationState(),
				}
			},
			expectedOp: models.ManagementInstanceOperation{
				ID:          "op id",
				Status:      models.InstanceOperationStatusOK,
				Explanation: "it's a decription",
				Log:         " 1 test step: okay",
				State: models.DefaultOperationState().
					SetMoveInstanceStepState(&models.MoveInstanceStepState{
						TaskID: "123qwe",
					}),
			},
		},
		{
			name: "1st ok, 2nd temporary fail",
			stepFactory: func(ctrl *gomock.Controller) []steps.InstanceStep {
				step1 := stepmocks.NewMockInstanceStep(ctrl)
				step1.EXPECT().Name().AnyTimes().Return("good step")
				step1.EXPECT().RunStep(gomock.Any(), gomock.Any(), gomock.Any()).Return(steps.RunResult{
					Log:         []string{"okay"},
					Description: "well done",
					IsDone:      true,
				})

				step2 := stepmocks.NewMockInstanceStep(ctrl)
				step2.EXPECT().Name().AnyTimes().Return("bad step")
				step2.EXPECT().RunStep(gomock.Any(), gomock.Any(), gomock.Any()).Return(steps.RunResult{
					Log:         []string{"try", "try"},
					Description: "smth went wrong",
					Error:       xerrors.New("bad boy"),
				})
				return []steps.InstanceStep{step1, step2}
			},
			cleanupStepFactory: func(ctrl *gomock.Controller) []steps.InstanceStep {
				return nil
			},
			opFactory: func() models.ManagementInstanceOperation {
				return models.ManagementInstanceOperation{
					ID:         "op id",
					Type:       models.InstanceOperationMove,
					Status:     models.InstanceOperationStatusNew,
					InstanceID: "fqdn id",
					State:      models.DefaultOperationState(),
				}
			},
			expectedOp: models.ManagementInstanceOperation{
				ID:          "op id",
				Status:      models.InstanceOperationStatusInProgress,
				Explanation: "smth went wrong",
				Log: " 1 good step: okay\n" +
					" 2 bad step: try\n" +
					"             try\n" +
					"             an error happened: bad boy\n" +
					"             I will try again",
				State: models.DefaultOperationState(),
			},
		},
		{
			name: "rejected, but cannot cleanup yet",
			stepFactory: func(ctrl *gomock.Controller) []steps.InstanceStep {
				step := stepmocks.NewMockInstanceStep(ctrl)
				step.EXPECT().Name().AnyTimes().Return("bad step")
				step.EXPECT().RunStep(gomock.Any(), gomock.Any(), gomock.Any()).Return(steps.RunResult{
					Log:         []string{"try", "try"},
					Description: "smth went wrong",
					Error:       xerrors.New("step test error"),
					IsDone:      true,
				})
				return []steps.InstanceStep{step}
			},
			cleanupStepFactory: func(ctrl *gomock.Controller) []steps.InstanceStep {
				step := stepmocks.NewMockInstanceStep(ctrl)
				step.EXPECT().Name().AnyTimes().Return("cleanup step")
				step.EXPECT().RunStep(gomock.Any(), gomock.Any(), gomock.Any()).Return(steps.RunResult{
					Log:         []string{"try", "try"},
					Description: "smth went wrong while cleanup",
					Error:       xerrors.New("cleanup temporary error"),
					IsDone:      false,
				})
				return []steps.InstanceStep{step}
			},
			opFactory: func() models.ManagementInstanceOperation {
				return models.ManagementInstanceOperation{
					ID:         "op id",
					Type:       models.InstanceOperationMove,
					Status:     models.InstanceOperationStatusNew,
					InstanceID: "fqdn id",
					State:      models.DefaultOperationState(),
				}
			},
			expectedOp: models.ManagementInstanceOperation{
				ID:          "op id",
				Status:      models.InstanceOperationStatusRejectPending,
				Explanation: "smth went wrong",
				Log: " 1 bad step: try\n" +
					"             try\n" +
					"             an error happened: step test error\n" +
					"             Will not retry, this is the final message\n" +
					" 2 cleanup step: try\n" +
					"                 try\n" +
					"                 an error happened: cleanup temporary error\n" +
					"                 I will try again",
				State: models.DefaultOperationState(),
			},
		},
		{
			name: "fail forever",
			stepFactory: func(ctrl *gomock.Controller) []steps.InstanceStep {
				step := stepmocks.NewMockInstanceStep(ctrl)
				step.EXPECT().Name().AnyTimes().Return("bad step")
				step.EXPECT().RunStep(gomock.Any(), gomock.Any(), gomock.Any()).Return(steps.RunResult{
					Log:         []string{"try", "try"},
					Description: "smth went wrong",
					Error:       xerrors.New("bad boy"),
					IsDone:      true,
				})
				return []steps.InstanceStep{step}
			},
			cleanupStepFactory: func(ctrl *gomock.Controller) []steps.InstanceStep {
				return nil
			},
			opFactory: func() models.ManagementInstanceOperation {
				return models.ManagementInstanceOperation{
					ID:         "op id",
					Type:       models.InstanceOperationMove,
					Status:     models.InstanceOperationStatusNew,
					InstanceID: "fqdn id",
					State:      models.DefaultOperationState(),
				}
			},
			expectedOp: models.ManagementInstanceOperation{
				ID:          "op id",
				Status:      models.InstanceOperationStatusRejected,
				Explanation: "smth went wrong",
				Log: " 1 bad step: try\n" +
					"             try\n" +
					"             an error happened: bad boy\n" +
					"             Will not retry, this is the final message",
				State: models.DefaultOperationState(),
			},
		},
		{
			name: "simple success cleanup",
			stepFactory: func(ctrl *gomock.Controller) []steps.InstanceStep {
				step := stepmocks.NewMockInstanceStep(ctrl)
				step.EXPECT().Name().AnyTimes().Return("test step")
				step.EXPECT().RunStep(gomock.Any(), gomock.Any(), gomock.Any()).DoAndReturn(
					func(ctx context.Context, stepCtx *opcontext.OperationContext, l log.Logger) steps.RunResult {
						stepCtx.State().MoveInstanceStep.TaskID = "123qwe"
						res := steps.RunResult{
							Log:         []string{"okay"},
							Description: "it's a decription",
							IsDone:      true,
						}
						return res
					})
				return []steps.InstanceStep{step}
			},
			cleanupStepFactory: func(ctrl *gomock.Controller) []steps.InstanceStep {
				step := stepmocks.NewMockInstanceStep(ctrl)
				step.EXPECT().Name().AnyTimes().Return("cleanup step")
				step.EXPECT().RunStep(gomock.Any(), gomock.Any(), gomock.Any()).DoAndReturn(
					func(ctx context.Context, stepCtx *opcontext.OperationContext, l log.Logger) steps.RunResult {
						res := steps.RunResult{
							Log:         []string{"cleaned"},
							Description: "everything is clean",
							IsDone:      true,
						}
						return res
					})
				return []steps.InstanceStep{step}
			},
			opFactory: func() models.ManagementInstanceOperation {
				return models.ManagementInstanceOperation{
					ID:         "op id",
					Type:       models.InstanceOperationMove,
					Status:     models.InstanceOperationStatusNew,
					InstanceID: "fqdn id",
					State:      models.DefaultOperationState(),
				}
			},
			expectedOp: models.ManagementInstanceOperation{
				ID:          "op id",
				Status:      models.InstanceOperationStatusOK,
				Explanation: "it's a decription",
				Log: " 1 test step: okay\n" +
					" 2 cleanup step: cleaned",
				State: models.DefaultOperationState().
					SetMoveInstanceStepState(&models.MoveInstanceStepState{
						TaskID: "123qwe",
					}),
			},
		},
		{
			name: "1st ok, 2nd temporary fail, do not clean",
			stepFactory: func(ctrl *gomock.Controller) []steps.InstanceStep {
				step1 := stepmocks.NewMockInstanceStep(ctrl)
				step1.EXPECT().Name().AnyTimes().Return("good step")
				step1.EXPECT().RunStep(gomock.Any(), gomock.Any(), gomock.Any()).Return(steps.RunResult{
					Log:         []string{"okay"},
					Description: "well done",
					IsDone:      true,
				})

				step2 := stepmocks.NewMockInstanceStep(ctrl)
				step2.EXPECT().Name().AnyTimes().Return("bad step")
				step2.EXPECT().RunStep(gomock.Any(), gomock.Any(), gomock.Any()).Return(steps.RunResult{
					Log:         []string{"try", "try"},
					Description: "smth went wrong",
					Error:       xerrors.New("bad boy"),
				})
				return []steps.InstanceStep{step1, step2}
			},
			cleanupStepFactory: func(ctrl *gomock.Controller) []steps.InstanceStep {
				step := stepmocks.NewMockInstanceStep(ctrl)
				step.EXPECT().Name().AnyTimes().Return("cleanup step")
				return []steps.InstanceStep{step}
			},
			opFactory: func() models.ManagementInstanceOperation {
				return models.ManagementInstanceOperation{
					ID:         "op id",
					Type:       models.InstanceOperationMove,
					Status:     models.InstanceOperationStatusNew,
					InstanceID: "fqdn id",
					State:      models.DefaultOperationState(),
				}
			},
			expectedOp: models.ManagementInstanceOperation{
				ID:          "op id",
				Status:      models.InstanceOperationStatusInProgress,
				Explanation: "smth went wrong",
				Log: " 1 good step: okay\n" +
					" 2 bad step: try\n" +
					"             try\n" +
					"             an error happened: bad boy\n" +
					"             I will try again",
				State: models.DefaultOperationState(),
			},
		},
		{
			name: "1st ok, 2nd fail, 3d not executed, cleanup",
			stepFactory: func(ctrl *gomock.Controller) []steps.InstanceStep {
				step1 := stepmocks.NewMockInstanceStep(ctrl)
				step1.EXPECT().Name().AnyTimes().Return("good step")
				step1.EXPECT().RunStep(gomock.Any(), gomock.Any(), gomock.Any()).Return(steps.RunResult{
					Log:         []string{"okay"},
					Description: "well done",
					IsDone:      true,
				})

				step2 := stepmocks.NewMockInstanceStep(ctrl)
				step2.EXPECT().Name().AnyTimes().Return("bad step")
				step2.EXPECT().RunStep(gomock.Any(), gomock.Any(), gomock.Any()).Return(steps.RunResult{
					Log:         []string{"try", "try"},
					Description: "smth went wrong",
					Error:       xerrors.New("bad boy"),
					IsDone:      true,
				})

				step3 := stepmocks.NewMockInstanceStep(ctrl)
				step3.EXPECT().Name().AnyTimes().Return("good step")
				return []steps.InstanceStep{step1, step2, step3}
			},
			cleanupStepFactory: func(ctrl *gomock.Controller) []steps.InstanceStep {
				step := stepmocks.NewMockInstanceStep(ctrl)
				step.EXPECT().Name().AnyTimes().Return("cleanup step")
				step.EXPECT().RunStep(gomock.Any(), gomock.Any(), gomock.Any()).DoAndReturn(
					func(ctx context.Context, stepCtx *opcontext.OperationContext, l log.Logger) steps.RunResult {
						res := steps.RunResult{
							Log:         []string{"cleaned"},
							Description: "everything is clean",
							IsDone:      true,
						}
						return res
					})
				return []steps.InstanceStep{step}
			},
			opFactory: func() models.ManagementInstanceOperation {
				return models.ManagementInstanceOperation{
					ID:         "op id",
					Type:       models.InstanceOperationMove,
					Status:     models.InstanceOperationStatusNew,
					InstanceID: "fqdn id",
					State:      models.DefaultOperationState(),
				}
			},
			expectedOp: models.ManagementInstanceOperation{
				ID:          "op id",
				Status:      models.InstanceOperationStatusRejected,
				Explanation: "smth went wrong",
				Log: " 1 good step: okay\n" +
					" 2 bad step: try\n" +
					"             try\n" +
					"             an error happened: bad boy\n" +
					"             Will not retry, this is the final message\n" +
					" 3 cleanup step: cleaned",
				State: models.DefaultOperationState(),
			},
		},
		{
			name: "simple success, temporary failed cleanup",
			stepFactory: func(ctrl *gomock.Controller) []steps.InstanceStep {
				step := stepmocks.NewMockInstanceStep(ctrl)
				step.EXPECT().Name().AnyTimes().Return("test step")
				step.EXPECT().RunStep(gomock.Any(), gomock.Any(), gomock.Any()).DoAndReturn(
					func(ctx context.Context, stepCtx *opcontext.OperationContext, l log.Logger) steps.RunResult {
						stepCtx.State().MoveInstanceStep.TaskID = "123qwe"
						res := steps.RunResult{
							Log:         []string{"okay"},
							Description: "it's a decription",
							IsDone:      true,
						}
						return res
					})
				return []steps.InstanceStep{step}
			},
			cleanupStepFactory: func(ctrl *gomock.Controller) []steps.InstanceStep {
				step := stepmocks.NewMockInstanceStep(ctrl)
				step.EXPECT().Name().AnyTimes().Return("cleanup step")
				step.EXPECT().RunStep(gomock.Any(), gomock.Any(), gomock.Any()).DoAndReturn(
					func(ctx context.Context, stepCtx *opcontext.OperationContext, l log.Logger) steps.RunResult {
						res := steps.RunResult{
							Log:         []string{"clean?"},
							Description: "need clean retry",
							IsDone:      false,
						}
						return res
					})
				return []steps.InstanceStep{step}
			},
			opFactory: func() models.ManagementInstanceOperation {
				return models.ManagementInstanceOperation{
					ID:         "op id",
					Type:       models.InstanceOperationMove,
					Status:     models.InstanceOperationStatusNew,
					InstanceID: "fqdn id",
					State:      models.DefaultOperationState(),
				}
			},
			expectedOp: models.ManagementInstanceOperation{
				ID:          "op id",
				Status:      models.InstanceOperationStatusOkPending,
				Explanation: "it's a decription",
				Log: " 1 test step: okay\n" +
					" 2 cleanup step: clean?\n" +
					"                 Need more time, I will try again later",
				State: models.DefaultOperationState().
					SetMoveInstanceStepState(&models.MoveInstanceStepState{
						TaskID: "123qwe",
					}),
			},
		},
		{
			name: "simple success, failed forever cleanup",
			stepFactory: func(ctrl *gomock.Controller) []steps.InstanceStep {
				step := stepmocks.NewMockInstanceStep(ctrl)
				step.EXPECT().Name().AnyTimes().Return("test step")
				step.EXPECT().RunStep(gomock.Any(), gomock.Any(), gomock.Any()).DoAndReturn(
					func(ctx context.Context, stepCtx *opcontext.OperationContext, l log.Logger) steps.RunResult {
						stepCtx.State().MoveInstanceStep.TaskID = "123qwe"
						res := steps.RunResult{
							Log:         []string{"okay"},
							Description: "it's a decription",
							IsDone:      true,
						}
						return res
					})
				return []steps.InstanceStep{step}
			},
			cleanupStepFactory: func(ctrl *gomock.Controller) []steps.InstanceStep {
				step := stepmocks.NewMockInstanceStep(ctrl)
				step.EXPECT().Name().AnyTimes().Return("cleanup step")
				step.EXPECT().RunStep(gomock.Any(), gomock.Any(), gomock.Any()).DoAndReturn(
					func(ctx context.Context, stepCtx *opcontext.OperationContext, l log.Logger) steps.RunResult {
						res := steps.RunResult{
							Log:         []string{"clean?"},
							Description: "it's a fail =(",
							IsDone:      true,
							Error:       xerrors.New("some error"),
						}
						return res
					})
				return []steps.InstanceStep{step}
			},
			opFactory: func() models.ManagementInstanceOperation {
				return models.ManagementInstanceOperation{
					ID:         "op id",
					Type:       models.InstanceOperationMove,
					Status:     models.InstanceOperationStatusNew,
					InstanceID: "fqdn id",
					State:      models.DefaultOperationState(),
				}
			},
			expectedOp: models.ManagementInstanceOperation{
				ID:          "op id",
				Status:      models.InstanceOperationStatusRejected,
				Explanation: "it's a decription",
				Log: " 1 test step: okay\n" +
					" 2 cleanup step: clean?\n" +
					"                 an error happened: some error\n" +
					"                 Will not retry, this is the final message",
				State: models.DefaultOperationState().
					SetMoveInstanceStepState(&models.MoveInstanceStepState{
						TaskID: "123qwe",
					}),
			},
		},
		{
			name: "2 steps full",
			stepFactory: func(ctrl *gomock.Controller) []steps.InstanceStep {
				step1 := stepmocks.NewMockInstanceStep(ctrl)
				step1.EXPECT().Name().AnyTimes().Return("first step")
				step1.EXPECT().RunStep(gomock.Any(), gomock.Any(), gomock.Any()).Return(steps.RunResult{
					Log:         []string{"okay"},
					Description: "it's a decription one",
					IsDone:      true,
				})
				step2 := stepmocks.NewMockInstanceStep(ctrl)
				step2.EXPECT().Name().AnyTimes().Return("second step")
				step2.EXPECT().RunStep(gomock.Any(), gomock.Any(), gomock.Any()).Return(steps.RunResult{
					Log:         []string{"okay"},
					Description: "it's a decription two",
					IsDone:      true,
				})
				return []steps.InstanceStep{step1, step2}
			},
			cleanupStepFactory: func(ctrl *gomock.Controller) []steps.InstanceStep {
				step := stepmocks.NewMockInstanceStep(ctrl)
				step.EXPECT().Name().AnyTimes().Return("cleanup step")
				step.EXPECT().RunStep(gomock.Any(), gomock.Any(), gomock.Any()).Return(steps.RunResult{
					Log:         []string{"cleaned"},
					Description: "everything is clean",
					IsDone:      true,
				})
				return []steps.InstanceStep{step}
			},
			opFactory: func() models.ManagementInstanceOperation {
				return models.ManagementInstanceOperation{
					ID:         "op id",
					Type:       models.InstanceOperationMove,
					Status:     models.InstanceOperationStatusNew,
					InstanceID: "fqdn id",
					State:      models.DefaultOperationState(),
				}
			},
			expectedOp: models.ManagementInstanceOperation{
				ID:          "op id",
				Status:      models.InstanceOperationStatusOK,
				Explanation: "it's a decription two",
				Log: " 1 first step: okay\n" +
					" 2 second step: okay\n" +
					" 3 cleanup step: cleaned",
				State: models.DefaultOperationState(),
			},
		},
		{
			name: "2 steps, 1st with finish workflow",
			stepFactory: func(ctrl *gomock.Controller) []steps.InstanceStep {
				step1 := stepmocks.NewMockInstanceStep(ctrl)
				step1.EXPECT().Name().AnyTimes().Return("first step")
				step1.EXPECT().RunStep(gomock.Any(), gomock.Any(), gomock.Any()).Return(steps.RunResult{
					Log:            []string{"okay"},
					Description:    "it's a decription one",
					IsDone:         true,
					FinishWorkflow: true,
				})
				step2 := stepmocks.NewMockInstanceStep(ctrl)
				step2.EXPECT().Name().AnyTimes().Return("second step")
				step2.EXPECT().RunStep(gomock.Any(), gomock.Any(), gomock.Any()).AnyTimes().Return(steps.RunResult{
					Log:         []string{"okay"},
					Description: "it's a decription two",
					IsDone:      true,
				})
				return []steps.InstanceStep{step1, step2}
			},
			cleanupStepFactory: func(ctrl *gomock.Controller) []steps.InstanceStep {
				step := stepmocks.NewMockInstanceStep(ctrl)
				step.EXPECT().Name().AnyTimes().Return("cleanup step")
				step.EXPECT().RunStep(gomock.Any(), gomock.Any(), gomock.Any()).Return(steps.RunResult{
					Log:         []string{"cleaned"},
					Description: "everything is clean",
					IsDone:      true,
				})
				return []steps.InstanceStep{step}
			},
			opFactory: func() models.ManagementInstanceOperation {
				return models.ManagementInstanceOperation{
					ID:         "op id",
					Type:       models.InstanceOperationMove,
					Status:     models.InstanceOperationStatusNew,
					InstanceID: "fqdn id",
					State:      models.DefaultOperationState(),
				}
			},
			expectedOp: models.ManagementInstanceOperation{
				ID:          "op id",
				Status:      models.InstanceOperationStatusOK,
				Explanation: "it's a decription one",
				Log: " 1 first step: okay\n" +
					"               Workflow will be finished by this step\n" +
					" 2 cleanup step: cleaned",
				State: models.DefaultOperationState(),
			},
		},
	}

	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			ctrl := gomock.NewController(t)
			defer ctrl.Finish()
			cmsdb := cmsdbmocks.NewMockClient(ctrl)
			cmsdb.EXPECT().UpdateInstanceOperationFields(gomock.Any(), gomock.Any()).DoAndReturn(
				func(_ context.Context, op models.ManagementInstanceOperation) error {
					tc.savedOp = op
					return nil
				}).AnyTimes()

			logger, _ := zap.New(zap.ConsoleConfig(log.DebugLevel))
			wf := workflow.NewBaseWorkflow(cmsdb, logger, tc.name, tc.stepFactory(ctrl), tc.cleanupStepFactory(ctrl))

			ctx := context.Background()
			var wg sync.WaitGroup
			wg.Add(1)
			tctx, cancel := context.WithTimeout(ctx, 10*time.Second)
			defer cancel()
			workflow.RunWorkflow(tctx, &wg, opcontext.NewStepContext(tc.opFactory()), wf)
			wg.Wait()

			require.Equal(t, tc.expectedOp.ID, tc.savedOp.ID)
			require.Equal(t, tc.expectedOp.Status, tc.savedOp.Status)
			require.Equal(t, tc.expectedOp.Explanation, tc.savedOp.Explanation)
			require.Equal(t, tc.expectedOp.Log, tc.savedOp.Log)
			require.Equal(t, tc.expectedOp.State, tc.savedOp.State)
		})
	}
}
