package metrics

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/settings"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
)

type HandledCounter struct {
	DutyHandled int
	All         int
}

func (hc *HandledCounter) Percents() float64 {
	if hc.All == 0 {
		return 0
	}
	return 100 * float64(hc.DutyHandled) / float64(hc.All)
}

type MetricAutoDutySuccessRate map[string]*HandledCounter

func (asr MetricAutoDutySuccessRate) All() *HandledCounter {
	result := HandledCounter{}
	for _, v := range asr {
		result.DutyHandled += v.DutyHandled
		result.All += v.All
	}
	return &result
}

func (i *Interactor) AutoDutySuccessRate(ctx context.Context) (MetricAutoDutySuccessRate, error) {
	result := MetricAutoDutySuccessRate{}
	allRequests, err := i.cmsdb.GetRequestsStatInWindow(ctx, settings.MetricsWindow)
	if err != nil {
		return nil, err
	}
	for _, r := range allRequests {
		cnt, ok := result[r.Name]
		if !ok {
			cnt = &HandledCounter{
				All: 0, DutyHandled: 0,
			}
			result[r.Name] = cnt
		}
		cnt.All++
		if r.IsResolvedByAutoDuty() {
			cnt.DutyHandled++
		}
	}
	ctxlog.Infof(ctx, i.lg, "collected metrics for %d requests", len(allRequests))

	return result, nil
}
