package provider

import (
	"fmt"
	"strings"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
)

var (
	mapDayStringToISOWeekday = map[string]int{
		"MON": 1,
		"TUE": 2,
		"WED": 3,
		"THU": 4,
		"FRI": 5,
		"SAT": 6,
		"SUN": 7,
	}
)

func GetNewMaintenanceTime(maintenanceInfo clusters.MaintenanceInfo, rescheduleType clusters.RescheduleType, delayedUntil optional.Time) (time.Time, error) {
	var err error
	if !maintenanceInfo.Operation.Valid() {
		return time.Time{}, semerr.InvalidInput("There is no maintenance operation at this time")
	}
	newMaintenanceTime := maintenanceInfo.Operation.DelayedUntil
	switch rescheduleType {
	case clusters.RescheduleTypeUnspecified:
		return time.Time{}, semerr.InvalidInput("Reschedule type need to be specified")
	case clusters.RescheduleTypeImmediate:
		newMaintenanceTime = time.Now()
	case clusters.RescheduleTypeSpecificTime:
		newMaintenanceTime, err = delayedUntil.Get()
		if err != nil {
			return time.Time{}, semerr.InvalidInput("Delayed until time need to be specified with specific time rescheduling type")
		}
	case clusters.RescheduleTypeNextAvailableWindow:
		if maintenanceInfo.Window.Anytime() {
			return time.Time{}, semerr.InvalidInput("The window need to be specified with next available window rescheduling type")
		}
		newMaintenanceTime = *CalculateNearestMaintenanceWindow(
			maintenanceInfo.Operation.DelayedUntil,
			maintenanceInfo.Window,
		)
	}
	if newMaintenanceTime == maintenanceInfo.Operation.DelayedUntil {
		return time.Time{}, semerr.InvalidInput("No changes detected")
	}
	if newMaintenanceTime.After(maintenanceInfo.Operation.LatestMaintenanceTime) &&
		rescheduleType != clusters.RescheduleTypeImmediate {
		return time.Time{}, semerr.InvalidInput(fmt.Sprintf("Maintenance operation time cannot exceed %s", maintenanceInfo.Operation.LatestMaintenanceTime.Format(time.RFC3339)))
	}

	return newMaintenanceTime, nil
}

func CalculateNearestMaintenanceWindow(operationTime time.Time, maintenanceWindow clusters.MaintenanceWindow) *time.Time {
	if maintenanceWindow.Anytime() {
		return nil
	}

	operationWeekday := mapDayStringToISOWeekday[strings.ToUpper(operationTime.Weekday().String()[:3])]
	windowWeekday := mapDayStringToISOWeekday[maintenanceWindow.Day]

	res := operationTime
	if operationWeekday == windowWeekday {
		if operationTime.Hour() >= maintenanceWindow.Hour-1 {
			res = res.AddDate(0, 0, 7)
		}
	} else if operationWeekday < windowWeekday {
		res = res.AddDate(0, 0, windowWeekday-operationWeekday)
	} else {
		res = res.AddDate(0, 0, 7+windowWeekday-operationWeekday)
	}

	res = time.Date(res.Year(), res.Month(), res.Day(), maintenanceWindow.Hour-1, res.Minute(), res.Second(), res.Nanosecond(), res.Location())

	return &res
}

func CalculateLatestMaintenanceTime(createdAt time.Time) time.Time {
	return createdAt.AddDate(0, 0, 7*3) // Add 3 weeks
}
