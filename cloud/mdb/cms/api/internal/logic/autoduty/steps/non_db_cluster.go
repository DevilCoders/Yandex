package steps

import (
	"context"
	"fmt"
	"strings"

	"a.yandex-team.ru/cloud/mdb/internal/conductor"
	"a.yandex-team.ru/library/go/slices"
)

type NonDatabaseClusterStep struct {
	cncl          conductor.Client
	takeNoMoreNum int
	groupName     string
}

func (s *NonDatabaseClusterStep) GetStepName() string {
	return "dom0 not for containers"
}

func (s *NonDatabaseClusterStep) hostsFromGroup(ctx context.Context) ([]string, error) {
	return s.cncl.GroupToHosts(ctx, s.groupName, conductor.GroupToHostsAttrs{})
}

func (s *NonDatabaseClusterStep) RunStep(ctx context.Context, execCtx *InstructionCtx) RunResult {
	rd := execCtx.GetActualRD()

	groupHosts, err := s.hostsFromGroup(ctx)
	if err != nil {
		return escalateWithErrAndMsg(err, "could not get list of hosts in Conductor")
	}
	if !slices.ContainsString(groupHosts, rd.R.MustOneFQDN()) {
		msg := fmt.Sprintf(
			"don't know about this host, try look at https://c.yandex-team.ru/api/hosts2groups/%s",
			rd.R.MustOneFQDN())
		return escalateWithMessage(msg)
	}

	var given []string
	for _, r := range execCtx.RequestsGivenAway() {
		if slices.ContainsString(groupHosts, r.MustOneFQDN()) {
			given = append(given, r.MustOneFQDN())
		}
	}
	givenNum := len(given)
	if givenNum < s.takeNoMoreNum {
		return approveWithMessage(fmt.Sprintf(
			"approve, %d given in group '%s' and allowed to give %d max",
			givenNum, s.groupName, s.takeNoMoreNum))
	} else {
		return waitWithMessage(fmt.Sprintf(
			"%d given to Wall-e from group '%s' (should be <= %d): %s",
			givenNum, s.groupName, s.takeNoMoreNum, strings.Join(given, ", ")))
	}
}

func NewNonDatabaseClusterStep(cncl conductor.Client, takeNoMoreNum int, groupName string) DecisionStep {
	return &NonDatabaseClusterStep{
		cncl:          cncl,
		takeNoMoreNum: takeNoMoreNum,
		groupName:     groupName,
	}
}
