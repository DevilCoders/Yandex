package steps

import (
	"context"
	"fmt"
)

type ApproveManualRequestStep struct {
}

func (s *ApproveManualRequestStep) GetStepName() string {
	return "approve if manual"
}

func (s *ApproveManualRequestStep) RunStep(ctx context.Context, execCtx *InstructionCtx) RunResult {
	rd := execCtx.GetActualRD()
	if rd.R.Type == "manual" {
		return approveWithMessage("approved")
	} else {
		return continueWithMessage(fmt.Sprintf("request type is '%s'", rd.R.Type))
	}
}

func NewApproveManualRequestStep() DecisionStep {
	return &ApproveManualRequestStep{}
}
