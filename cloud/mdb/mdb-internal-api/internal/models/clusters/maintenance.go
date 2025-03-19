package clusters

import (
	"reflect"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/semerr"
)

type WeeklyMaintenanceWindow struct {
	Day  string
	Hour int
}

type MaintenanceWindow struct {
	*WeeklyMaintenanceWindow
}

func (mw MaintenanceWindow) Anytime() bool {
	return mw.WeeklyMaintenanceWindow == nil
}

func (mw MaintenanceWindow) Validate() error {
	if mw.WeeklyMaintenanceWindow != nil {
		if mw.Hour < 1 || mw.Hour > 24 {
			return semerr.InvalidInput("Maintenance window hour should be in range (1 - 24)")
		}
	}

	return nil
}

func (mw MaintenanceWindow) Equal(o MaintenanceWindow) bool {
	return reflect.DeepEqual(mw, o)
}

func NewAnytimeMaintenanceWindow() MaintenanceWindow {
	return MaintenanceWindow{}
}

func NewWeeklyMaintenanceWindow(day string, hour int) MaintenanceWindow {
	return MaintenanceWindow{
		WeeklyMaintenanceWindow: &WeeklyMaintenanceWindow{
			Day:  day,
			Hour: hour,
		},
	}
}

type MaintenanceInfo struct {
	ClusterID string
	Window    MaintenanceWindow
	Operation MaintenanceOperation
}

type MaintenanceOperation struct {
	ConfigID                 string
	CreatedAt                time.Time
	DelayedUntil             time.Time
	Info                     string
	NearestMaintenanceWindow *time.Time
	LatestMaintenanceTime    time.Time

	valid bool
}

func (mo MaintenanceOperation) Valid() bool {
	return mo.valid
}

func NewMaintenanceOperation(configID string, createdAt, delayedUntil time.Time, info string) MaintenanceOperation {
	return MaintenanceOperation{
		ConfigID:     configID,
		CreatedAt:    createdAt,
		DelayedUntil: delayedUntil,
		Info:         info,

		valid: true,
	}
}

// RescheduleType for maintenance reschedule tasks
type RescheduleType int

const (
	RescheduleTypeUnspecified RescheduleType = iota
	RescheduleTypeImmediate
	RescheduleTypeNextAvailableWindow
	RescheduleTypeSpecificTime
)
