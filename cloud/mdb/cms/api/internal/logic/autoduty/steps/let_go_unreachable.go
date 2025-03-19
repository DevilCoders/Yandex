package steps

import (
	"context"
	"fmt"
	"strings"
	"time"

	juggler2 "a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/helpers/juggler"
	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/models"
	"a.yandex-team.ru/cloud/mdb/internal/juggler"
)

type ActOnUnreachableStep struct {
	waitStableTime time.Duration
	action         AfterStepAction
	reachability   juggler2.JugglerChecker
}

func (s *ActOnUnreachableStep) GetStepName() string {
	return "is dom0 unreachable?"
}

func WalleConsidersUnreachable(r models.ManagementRequest) bool {
	return strings.Contains(strings.ToLower(r.Comment), "unreachable") ||
		strings.Contains(strings.ToLower(r.Comment), "host is not available: ssh")
}

func (s *ActOnUnreachableStep) RunStep(ctx context.Context, execCtx *InstructionCtx) RunResult {
	rd := execCtx.GetActualRD()
	dom0 := rd.R.MustOneFQDN()
	reachableInfo, err := s.reachability.Check(ctx, []string{dom0}, []string{juggler2.Unreachable}, time.Now())
	if err != nil {
		return waitWithErrAndMessage(err, "error in juggler, will try again")
	}
	if !reachableInfo.IsOK(dom0) {
		return RunResult{
			ForHuman: "yes, unreachable in juggler",
			Action:   s.action,
		}
	}
	if !WalleConsidersUnreachable(rd.R) {
		return continueWithMessage("no")
	}
	return WaitTillTimeElapses(time.Now(), rd.R.CreatedAt, s.waitStableTime, s.action)
}

func WaitTillTimeElapses(now, from time.Time, waitFor time.Duration, action AfterStepAction) RunResult {
	if now.After(from.Add(waitFor)) {
		return RunResult{
			ForHuman: fmt.Sprintf("waited %s", waitFor.Round(time.Second)),
			Action:   action,
		}
	}
	return waitWithMessage(fmt.Sprintf("will wait till %s", from.Add(waitFor).Round(time.Second)))

}

func NewActOnUnreachableStep(waitStableTime time.Duration, action AfterStepAction, jgrl juggler.API, unreachWindow int) DecisionStep {
	return NewCustomActOnUnreachableStep(waitStableTime, juggler2.NewJugglerReachabilityChecker(jgrl, unreachWindow), action)
}

func NewCustomActOnUnreachableStep(
	waitStableTime time.Duration,
	reachability juggler2.JugglerChecker,
	action AfterStepAction,
) DecisionStep {
	return &ActOnUnreachableStep{
		waitStableTime: waitStableTime,
		action:         action,
		reachability:   reachability,
	}
}
