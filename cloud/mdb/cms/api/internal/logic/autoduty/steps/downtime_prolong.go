package steps

import (
	"context"
	"fmt"
	"time"

	"github.com/opentracing/opentracing-go"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/dom0discovery"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/models"
	"a.yandex-team.ru/cloud/mdb/internal/juggler"
)

type ProlongDowntimesStep struct {
	dom0d dom0discovery.Dom0Discovery
	jglr  juggler.API
}

func (s *ProlongDowntimesStep) GetStepName() string {
	return "prolong downtimes"
}

func prolongDowntimesByContainerList(ctx context.Context, containers []models.Instance, jglr juggler.API, dtIDs []string, prolongUpTo time.Time) error {
	span, ctx := opentracing.StartSpanFromContext(ctx, "prolongDowntimesByContainerList")
	defer span.Finish()
	filters := make([]juggler.DowntimeFilter, len(dtIDs))
	for i, id := range dtIDs {
		filters[i] = juggler.DowntimeFilter{DowntimeID: id}
	}
	downtimes, err := jglr.GetDowntimes(ctx, juggler.Downtime{
		Filters: filters,
	})
	if err != nil {
		return fmt.Errorf("could not get downtimes: %w", err)
	}

	containerFqdns := make(map[string]bool)
	for _, container := range containers {
		containerFqdns[container.FQDN] = true
	}
	for _, dt := range downtimes {
		var prolongDtFilters []juggler.DowntimeFilter
		for _, filter := range dt.Filters {
			if _, ok := containerFqdns[filter.Host]; ok {
				prolongDtFilters = append(prolongDtFilters, filter)
			}
		}

		if len(prolongDtFilters) != 0 {
			dt.Filters = prolongDtFilters
			dt.EndTime = prolongUpTo
			_, err = jglr.SetDowntimes(ctx, dt)
			if err != nil {
				return fmt.Errorf("could not set downtime %s: %w", dt.DowntimeID, err)
			}
		}
	}
	return nil
}

func (s *ProlongDowntimesStep) RunStep(ctx context.Context, execCtx *InstructionCtx) RunResult {
	rd := execCtx.GetActualRD()
	containers, err := s.dom0d.Dom0Instances(ctx, rd.R.MustOneFQDN())
	if err != nil {
		return waitWithErrAndMessage(err, "could not get list of containers on dom0")
	}
	if len(containers.WellKnown) == 0 {
		return continueWithMessage("nothing to do")
	}

	latest := rd.D.OpsLog.Downtimes.Latest()
	endTime := time.Now().Add(time.Hour)
	if latest == nil {
		return continueWithMessage("no downtimes remembered")
	} else {
		err := prolongDowntimesByContainerList(ctx, containers.WellKnown, s.jglr, latest.DowntimeIDs, endTime)
		if err != nil {
			return waitWithMessage(err.Error())
		}
	}
	return continueWithMessage(fmt.Sprintf("downtimes prolonged +%s", endTime.Round(time.Second)))
}

func NewProlongDowntimesStep(jglr juggler.API, dom0d dom0discovery.Dom0Discovery) DecisionStep {
	return &ProlongDowntimesStep{jglr: jglr, dom0d: dom0d}
}
