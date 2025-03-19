package steps_test

import (
	"fmt"
	"testing"

	"github.com/stretchr/testify/require"

	swagm "a.yandex-team.ru/cloud/mdb/cms/api/generated/swagger/models"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/steps"
	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/models"
)

func TestMemoryStep(t *testing.T) {
	for ind, ftype := range []string{
		steps.FailureTypeMemEcc,
		steps.FailureTypeMemSize,
		steps.FailureTypeMemNuma,
		steps.FailureTypeMemSpeed,
	} {
		t.Run(fmt.Sprintf("mem condition %d", ind), func(t *testing.T) {
			res := steps.CheckConditions(models.ManagementRequest{
				Type:        swagm.ManagementRequestTypeAutomated,
				FailureType: ftype,
			})
			require.Equal(t, steps.AfterStepApprove, res.Action)
			expectedMsg := fmt.Sprintf("yes, replace memory (failure type: %s)", ftype)
			require.Equal(t, expectedMsg, res.ForHuman)
		})
	}
	for ind, ftype := range []string{
		steps.FailureTypeCPUOverheated,
		steps.FailureTypeCPUFailure,
	} {
		t.Run(fmt.Sprintf("cpu condition %d", ind), func(t *testing.T) {
			res := steps.CheckConditions(models.ManagementRequest{
				Type:        swagm.ManagementRequestTypeAutomated,
				FailureType: ftype,
			})
			require.Equal(t, steps.AfterStepApprove, res.Action)
			expectedMsg := fmt.Sprintf("yes, will replace thermal paste or CPU itself (failure type: %s)", ftype)
			require.Equal(t, expectedMsg, res.ForHuman)
		})
	}

	t.Run("battery condition", func(t *testing.T) {
		res := steps.CheckConditions(models.ManagementRequest{
			Type:        swagm.ManagementRequestTypeAutomated,
			FailureType: steps.FailureTypeBmcBattery,
		})
		require.Equal(t, steps.AfterStepApprove, res.Action)
		require.Equal(t, "yes, replace battery (failure type: bmc_battery)", res.ForHuman)
	})

	t.Run("manual request", func(t *testing.T) {
		res := steps.CheckConditions(models.ManagementRequest{
			Type:        swagm.ManagementRequestTypeManual,
			FailureType: "",
		})
		require.Equal(t, steps.AfterStepApprove, res.Action)
		require.Equal(t, "yes, request created manually", res.ForHuman)
	})

	for ind, ftype := range []string{
		"",       // empty failure type
		"qwerty", // unknown failure type
	} {
		t.Run(fmt.Sprintf("unhandled failure type %d", ind), func(t *testing.T) {
			res := steps.CheckConditions(models.ManagementRequest{
				Type:        swagm.ManagementRequestTypeAutomated,
				FailureType: ftype,
			})
			require.Equal(t, steps.AfterStepContinue, res.Action)
			expectedMsg := fmt.Sprintf("unhandlable failure type: %s", ftype)
			require.Equal(t, expectedMsg, res.ForHuman)
		})
	}

	type scenariosInput struct {
		t      string
		msg    string
		action steps.AfterStepAction
	}
	for _, input := range []scenariosInput{
		{
			t:      swagm.ManagementRequestScenarioInfoScenarioTypeItdcDashMaintenance,
			msg:    "yes, scenario: https://wall-e.yandex-team.ru/scenarios/42",
			action: steps.AfterStepApprove,
		},
		{
			t:      swagm.ManagementRequestScenarioInfoScenarioTypeNocDashHard,
			msg:    "yes, scenario: https://wall-e.yandex-team.ru/scenarios/42",
			action: steps.AfterStepApprove,
		},
		{
			t:      swagm.ManagementRequestScenarioInfoScenarioTypeNocDashSoft,
			msg:    "unhandlable failure type: ",
			action: steps.AfterStepContinue,
		},
		{
			t:      swagm.ManagementRequestScenarioInfoScenarioTypeEtc,
			msg:    "unhandlable failure type: ",
			action: steps.AfterStepContinue,
		},
	} {
		t.Run("scenarios", func(t *testing.T) {
			res := steps.CheckConditions(models.ManagementRequest{
				ScenarioInfo: models.ScenarioInfo{
					ID:   42,
					Type: input.t,
				},
				Type: swagm.ManagementRequestTypeAutomated,
			})
			require.Equal(t, input.action, res.Action)
			require.Equal(t, input.msg, res.ForHuman)
		})
	}
}
