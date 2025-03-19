package steps

import (
	"context"
	"time"
)

type SelfDNSConfig struct {
	WaitFor int `json:"wait_minutes" yaml:"wait_minutes"`
}

type WaitSelfDNSStep struct {
	waitFor time.Duration
}

func (s *WaitSelfDNSStep) GetStepName() string {
	return "self dns propagates new ips"
}

func (s *WaitSelfDNSStep) RunStep(ctx context.Context, execCtx *InstructionCtx) RunResult {
	rd := execCtx.GetActualRD()
	return WaitTillTimeElapses(time.Now(), rd.R.CameBackAt, s.waitFor, AfterStepContinue)
}

func NewWaitSelfDNSStep(cfg SelfDNSConfig) DecisionStep {
	return &WaitSelfDNSStep{
		waitFor: time.Minute * time.Duration(cfg.WaitFor),
	}
}
