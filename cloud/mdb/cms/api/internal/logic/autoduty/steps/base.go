package steps

import (
	"context"
)

//go:generate ../../../../../../scripts/mockgen.sh ReachabilityChecker

type DecisionStep interface {
	GetStepName() string
	RunStep(ctx context.Context, execCtx *InstructionCtx) RunResult
}

type Option func(step DecisionStep)
