package steps_test

import (
	"context"
	"testing"

	"github.com/golang/mock/gomock"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/instances/opcontext"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/instances/steps"
	tasksclientmocks "a.yandex-team.ru/cloud/mdb/cms/api/internal/tasksclient/mocks"
	tasksmodels "a.yandex-team.ru/cloud/mdb/cms/api/internal/tasksclient/models"
	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/models"
)

func TestMove(t *testing.T) {
	type testCase struct {
		name              string
		prepareTaskClient func(meta *tasksclientmocks.MockClient)
		expected          steps.RunResult
		stateFactory      func() *models.OperationState
		isCompute         bool
	}

	testCases := []testCase{
		{
			name: "wait task",
			prepareTaskClient: func(client *tasksclientmocks.MockClient) {
				client.EXPECT().CreateMoveInstanceTask(gomock.Any(), "fqdn1", "dom0").Return(
					"someID", nil,
				)
			},
			stateFactory: func() *models.OperationState {
				state := models.DefaultOperationState().
					SetFQDN("fqdn1").
					SetDom0FQDN("dom0")
				return state
			},
			expected: steps.RunResult{
				Description: "created worker task \"someID\"",
				Error:       nil,
				IsDone:      false,
			},
		},
		{
			name: "success",
			prepareTaskClient: func(client *tasksclientmocks.MockClient) {
				client.EXPECT().TaskStatus(gomock.Any(), "qwe").Return(
					tasksmodels.TaskStatusDone,
					nil,
				)
			},
			stateFactory: func() *models.OperationState {
				state := models.DefaultOperationState().
					SetFQDN("fqdn1")
				state.MoveInstanceStep.TaskID = "qwe"
				return state
			},
			expected: steps.RunResult{
				Description: "instance was successfully moved by worker task \"qwe\"",
				IsDone:      true,
			},
		},
		{
			name: "operation is not finished",
			prepareTaskClient: func(client *tasksclientmocks.MockClient) {
				client.EXPECT().TaskStatus(gomock.Any(), "qwe").Return(
					tasksmodels.TaskStatusRunning,
					nil,
				)
			},
			stateFactory: func() *models.OperationState {
				state := models.DefaultOperationState().
					SetFQDN("fqdn1")
				state.MoveInstanceStep.TaskID = "qwe"
				return state
			},
			expected: steps.RunResult{
				Description: "worker task \"qwe\" is not finished yet",
				IsDone:      false,
			},
		},
		{
			name: "operation is failed",
			prepareTaskClient: func(client *tasksclientmocks.MockClient) {
				client.EXPECT().TaskStatus(gomock.Any(), "qwe").Return(
					tasksmodels.TaskStatusFailed,
					nil,
				)
			},
			stateFactory: func() *models.OperationState {
				state := models.DefaultOperationState().
					SetFQDN("fqdn1")
				state.MoveInstanceStep.TaskID = "qwe"
				return state
			},
			expected: steps.RunResult{
				Description: "worker task \"qwe\" has been failed, step is failed",
				IsDone:      false,
			},
		},
	}

	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			ctx := context.Background()
			ctrl := gomock.NewController(t)
			defer ctrl.Finish()
			taskCLient := tasksclientmocks.NewMockClient(ctrl)
			tc.prepareTaskClient(taskCLient)

			step := steps.NewMoveInstance(taskCLient, tc.isCompute)
			stepCtx := opcontext.NewStepContext(models.ManagementInstanceOperation{
				ID:         "qwe",
				InstanceID: "fqdn1",
				State:      tc.stateFactory(),
			})

			testStep(t, ctx, stepCtx, step, tc.expected)
		})
	}
}
