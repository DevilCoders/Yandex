package steps

import (
	"context"
	"fmt"
	"strings"
	"time"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/dom0discovery"
	opmetas2 "a.yandex-team.ru/cloud/mdb/cms/api/pkg/opmetas"
	"a.yandex-team.ru/cloud/mdb/internal/juggler"
)

type SetDowntimesStep struct {
	dom0d      dom0discovery.Dom0Discovery
	jglr       juggler.API
	namespaces []string
}

func (s *SetDowntimesStep) GetStepName() string {
	return "downtimes set"
}

func (s *SetDowntimesStep) RunStep(ctx context.Context, execCtx *InstructionCtx) RunResult {
	rd := execCtx.GetActualRD()
	containers, err := s.dom0d.Dom0Instances(ctx, rd.R.MustOneFQDN())
	if err != nil {
		return escalateWithErrAndMsg(err, "could not get list of containers on dom0")
	}
	if len(containers.WellKnown) == 0 {
		return continueWithMessage("nothing to do")
	}
	latest := rd.D.OpsLog.Downtimes.Latest()
	var dtIDs []string
	if latest == nil {
		opMeta := opmetas2.NewSetDowntimesStepMeta()
		for _, ns := range s.namespaces {
			var dtFilters []juggler.DowntimeFilter
			for _, item := range containers.WellKnown {
				dtFilters = append(dtFilters, juggler.DowntimeFilter{
					Host:      item.FQDN,
					Namespace: ns,
				})
			}
			// dont put too much data into single request or you get unexplained 400
			dtr := juggler.NewSetDowntimeRequestByDuration(
				fmt.Sprintf("CMS let go %d hosts on '%s' to Wall-e", len(containers.WellKnown), rd.R.MustOneFQDN()), time.Hour*18, dtFilters)
			dtID, err := s.jglr.SetDowntimes(ctx, dtr)
			if err != nil {
				return escalateWithErrAndMsg(err, fmt.Sprintf("could not set downtime for namespace %s", ns))
			}
			dtIDs = append(dtIDs, dtID)
			opMeta.SaveDT(dtID)
		}
		if err := rd.D.OpsLog.Add(opMeta); err != nil {
			return escalateWithErrAndMsg(err, "could not save op meta data")
		}
	} else {
		dtIDs = latest.DowntimeIDs
		err := prolongDowntimesByContainerList(ctx, containers.WellKnown, s.jglr, latest.DowntimeIDs, time.Now().Add(time.Hour))
		if err != nil {
			return waitWithMessage(err.Error())
		}
	}
	return continueWithMessage(fmt.Sprintf("downtime ids: %s", strings.Join(dtIDs, ", ")))

}

func NewSetDowntimesStep(dom0d dom0discovery.Dom0Discovery, jglr juggler.API, namespaces []string) DecisionStep {
	return &SetDowntimesStep{dom0d: dom0d, jglr: jglr, namespaces: namespaces}
}
