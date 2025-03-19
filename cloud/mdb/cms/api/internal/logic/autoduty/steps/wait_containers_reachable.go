package steps

import (
	"context"
	"fmt"
	"time"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/dom0discovery"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/helpers/juggler"
	jugglerapi "a.yandex-team.ru/cloud/mdb/internal/juggler"
	"a.yandex-team.ru/library/go/slices"
)

type WaitContainersReachableStep struct {
	reachability juggler.JugglerChecker
	dom0d        dom0discovery.Dom0Discovery
}

func (s *WaitContainersReachableStep) GetStepName() string {
	return "wait containers reachable"
}

func (s *WaitContainersReachableStep) RunStep(ctx context.Context, execCtx *InstructionCtx) RunResult {
	rd := execCtx.GetActualRD()
	dom0 := rd.R.MustOneFQDN()
	checkFQDNS := make([]string, 0)
	if containers, err := s.dom0d.Dom0Instances(ctx, dom0); err != nil {
		return errorListingDBM(err)
	} else {
		for _, cnt := range containers.WellKnown {
			checkFQDNS = append(checkFQDNS, cnt.FQDN)
		}
	}
	reachableInfo, err := s.reachability.Check(ctx, checkFQDNS, []string{juggler.Unreachable}, time.Now())
	if err != nil {
		return waitWithErrAndMessage(err, "error in juggler, will try again")
	}
	if len(reachableInfo.NotOK) > 0 {
		return waitWithMessage(fmt.Sprintf("%s unreachable in juggler, you should bring them up or use 'juggler_queue_event --host {fqdn} --service UNREACHABLE --status OK --description fake' on any dom0 to emulate liveness", slices.Join(reachableInfo.NotOK, ", ")))
	}
	return continueWithMessage("all reachable")
}

func NewWaitContainersAndDom0ReachableStep(
	jglr jugglerapi.API,
	dom0d dom0discovery.Dom0Discovery,
	jgrlConf juggler.JugglerConfig,
) DecisionStep {
	return &WaitContainersReachableStep{
		reachability: juggler.NewJugglerReachabilityChecker(jglr, jgrlConf.UnreachableServiceWindowMin),
		dom0d:        dom0d,
	}
}
