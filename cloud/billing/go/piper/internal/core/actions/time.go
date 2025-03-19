package actions

import (
	"time"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/core/entities"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/pkg/timetool"
)

// billMonthStartTime return month start time at billing timezone
func billMonthStartTime(locT time.Time) time.Time {
	return timetool.MonthStart(locT)
}

// nextBillMonthStartTime return next month start time at billing timezone
func nextBillMonthStartTime(locT time.Time) time.Time {
	return billMonthStartTime(locT).AddDate(0, 1, 0)
}

// billGraceTime return greatest month start time where `month_start_time + grace` less than given time
func billGraceTime(locT time.Time, grace time.Duration) time.Time {
	// find start of month which t belongs
	ms := billMonthStartTime(locT)
	graceTime := ms.Add(grace)
	if !locT.Before(graceTime) { // if grace ended - that is result
		return ms
	}
	// we are in grace from month start - get previous month
	return billMonthStartTime(ms.Add(-time.Hour * 24))
}

// MaxTime returns max(a,b)
func maxTime(a, b time.Time) time.Time {
	if a.After(b) {
		return a
	}
	return b
}

// MinTime returns min(a,b)
func minTime(a, b time.Time) time.Time {
	if a.Before(b) {
		return a
	}
	return b
}

func usageHours(u entities.MetricUsage) entities.MetricPeriod {
	startHour := u.UsageTime().Truncate(time.Hour)
	finishHour := u.Finish.Truncate(time.Hour)
	if finishHour.Before(u.Finish) {
		finishHour = finishHour.Add(time.Hour)
	}
	return entities.MetricPeriod{
		Start:  startHour,
		Finish: finishHour,
	}
}
