package steps

import (
	"context"
	"fmt"

	swagm "a.yandex-team.ru/cloud/mdb/cms/api/generated/swagger/models"
	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/models"
)

const (
	walleScenarioURL = "https://wall-e.yandex-team.ru/scenarios/"

	FailureTypeBmcBattery = "bmc_battery"

	FailureTypeMemEcc   = "mem_ecc"
	FailureTypeMemSpeed = "mem_speed"
	FailureTypeMemSize  = "mem_size"
	FailureTypeMemNuma  = "mem_numa"

	FailureTypeDiskBadCable = "disk_bad_cable"

	FailureTypeCPUOverheated = "cpu_overheated"
	FailureTypeCPUFailure    = "cpu_failure"
)

type LetGoSafePartChange struct {
}

func (s *LetGoSafePartChange) GetStepName() string {
	return "replace detail and return?"
}

func (s *LetGoSafePartChange) RunStep(ctx context.Context, execCtx *InstructionCtx) RunResult {
	rd := execCtx.GetActualRD()
	if rd.R.Name != "profile" {
		return continueWithMessage("step approves only 'profile' requests")
	}
	return CheckConditions(rd.R)
}

func CheckConditions(r models.ManagementRequest) RunResult {
	if r.Type == swagm.ManagementRequestTypeManual {
		return approveWithMessage("yes, request created manually")
	}

	switch r.ScenarioInfo.Type {
	case swagm.ManagementRequestScenarioInfoScenarioTypeItdcDashMaintenance,
		swagm.ManagementRequestScenarioInfoScenarioTypeNocDashHard:
		return approveWithMessage(fmt.Sprintf("yes, scenario: %s%d", walleScenarioURL, r.ScenarioInfo.ID))
	}

	switch r.FailureType {
	case FailureTypeCPUOverheated,
		FailureTypeCPUFailure:
		msg := fmt.Sprintf("yes, will replace thermal paste or CPU itself (failure type: %s)", r.FailureType)
		return approveWithMessage(msg)
	case FailureTypeMemEcc,
		FailureTypeMemSpeed,
		FailureTypeMemSize,
		FailureTypeMemNuma:
		msg := fmt.Sprintf("yes, replace memory (failure type: %s)", r.FailureType)
		return approveWithMessage(msg)
	case FailureTypeBmcBattery:
		msg := fmt.Sprintf("yes, replace battery (failure type: %s)", r.FailureType)
		return approveWithMessage(msg)
	case FailureTypeDiskBadCable:
		msg := fmt.Sprintf("yes, replace disk cable (failure type: %s)", r.FailureType)
		return approveWithMessage(msg)
	default:
		msg := fmt.Sprintf("unhandlable failure type: %s", r.FailureType)
		return continueWithMessage(msg)
	}
}

func NewLetGoOnSafePartChange() DecisionStep {
	return &LetGoSafePartChange{}
}
