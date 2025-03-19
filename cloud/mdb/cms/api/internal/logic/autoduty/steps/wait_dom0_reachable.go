package steps

import (
	"context"
	"fmt"
	"time"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/helpers/juggler"
	jugglerapi "a.yandex-team.ru/cloud/mdb/internal/juggler"
)

type WaitDom0ReachableStep struct {
	reachability juggler.JugglerChecker
}

func (s *WaitDom0ReachableStep) GetStepName() string {
	return "wait dom0 reachable"
}

func (s *WaitDom0ReachableStep) RunStep(ctx context.Context, execCtx *InstructionCtx) RunResult {
	rd := execCtx.GetActualRD()
	dom0 := rd.R.MustOneFQDN()
	checkFQDNS := []string{dom0}
	reachableInfo, err := s.reachability.Check(ctx, checkFQDNS, []string{juggler.Unreachable}, time.Now())
	if err != nil {
		return waitWithErrAndMessage(err, "error in juggler, will try again")
	}
	if len(reachableInfo.NotOK) > 0 {
		return waitWithMessage(fmt.Sprintf("%s is unreachable in juggler, you should bring it up", dom0))
	}
	return continueWithMessage("dom0 is reachable")
}

func NewWaitDom0ReachableStep(
	jglr jugglerapi.API,
	conf juggler.JugglerConfig,
) DecisionStep {
	return NewCustomWaitDom0ReachableStep(juggler.NewJugglerReachabilityChecker(jglr, conf.UnreachableServiceWindowMin))
}

func NewCustomWaitDom0ReachableStep(checker juggler.JugglerChecker) DecisionStep {
	return &WaitDom0ReachableStep{
		reachability: checker,
	}
}
