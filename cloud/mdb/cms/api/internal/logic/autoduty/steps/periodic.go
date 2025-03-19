package steps

import (
	"context"
	"fmt"
	"time"

	"github.com/jonboulle/clockwork"

	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/opmetas"
)

type Periodic struct {
	toRun func() []DecisionStep
	freq  time.Duration
	clock clockwork.Clock
}

func (s *Periodic) GetStepName() string {
	return "periodic"
}

func (s *Periodic) RunStep(ctx context.Context, execCtx *InstructionCtx) RunResult {
	rd := execCtx.GetActualRD()
	state := rd.D.OpsLog.PeriodicState
	if state == nil || s.clock.Since(state.LastRun.Time) >= s.freq {
		state = opmetas.NewPeriodicStateMeta(s.clock.Now())
		rd.D.OpsLog.PeriodicState = state
		return continueWithMessage("time to run", s.toRun()...)
	}
	return continueWithMessage(fmt.Sprintf("it's not time yet, last run was at %s", state.LastRun.String()))
}

func NewPeriodic(toRun func() []DecisionStep, freq time.Duration, opts ...Option) DecisionStep {
	res := &Periodic{toRun: toRun, freq: freq, clock: clockwork.NewRealClock()}
	for _, opt := range opts {
		opt(res)
	}
	return res
}

func PeriodicWithClock(clock clockwork.Clock) Option {
	return func(s DecisionStep) {
		s.(*Periodic).clock = clock
	}
}
